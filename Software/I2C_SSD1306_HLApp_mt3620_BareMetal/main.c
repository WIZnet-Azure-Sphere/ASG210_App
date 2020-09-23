/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

// This sample C application for Azure Sphere uses the Azure Sphere I2C APIs to display
// data from an accelerometer connected via I2C.
//
// It uses the APIs for the following Azure Sphere application libraries:
// - log (messages shown in Visual Studio's Device Output window during debugging)
// - i2c (communicates with LSM6DS3 accelerometer)
// - eventloop (system invokes handlers for timer events)

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/log.h>
#include <applibs/i2c.h>
#include <applibs/gpio.h>
#include <applibs/eventloop.h>

// The following #include imports a "sample appliance" definition. This app comes with multiple
// implementations of the sample appliance, each in a separate directory, which allow the code to
// run on different hardware.
//
// By default, this app targets hardware that follows the MT3620 Reference Development Board (RDB)
// specification, such as the MT3620 Dev Kit from Seeed Studio.
//
// To target different hardware, you'll need to update CMakeLists.txt. For example, to target the
// Avnet MT3620 Starter Kit, change the TARGET_DIRECTORY argument in the call to
// azsphere_target_hardware_definition to "HardwareDefinitions/avnet_mt3620_sk".
//
// See https://aka.ms/AzureSphereHardwareDefinitions for more details.
#include <hw/usi_mt3620_bt_combo.h>

#include "eventloop_timer_utilities.h"
#include "SSD1306.h"

/// <summary>
/// Exit codes for this application. These are used for the
/// application exit code. They must all be between zero and 255,
/// where zero is reserved for successful termination.
/// </summary>
typedef enum {
    ExitCode_Success = 0,

    ExitCode_TermHandler_SigTerm = 1,

    ExitCode_Init_EventLoop = 2,
    ExitCode_Init_OpenMaster = 3,
    ExitCode_Init_SetBusSpeed = 4,
    ExitCode_Init_SetTimeout = 5,
    ExitCode_Init_SetDefaultTarget = 6,
    
    ExitCode_Init_OpenButton = 7,
    ExitCode_Init_ButtonPollTimer = 8,
    ExitCode_Init_NSCL = 9,
    ExitCode_ButtonTimer_Consume = 10,
    ExitCode_ButtonTimer_GetValue = 11,

    ExitCode_Main_EventLoopFail = 12
} ExitCode;

// Support functions.
static void TerminationHandler(int signalNumber);
static bool CheckTransferSize(const char *desc, size_t expectedBytes, ssize_t actualBytes);
static ExitCode InitPeripheralsAndHandlers(void);
static void CloseFdAndPrintError(int fd, const char *fdName);
static void ClosePeripheralsAndHandlers(void);
static void ButtonTimerEventHandler(EventLoopTimer *timer);

// File descriptors - initialized to invalid value
static int i2cFd = -1;
static int gpioButtonFd = -1;
static int gpioNRS232Fd = -1;
static int gpioNSCL = -1;

static EventLoop *eventLoop = NULL;
static EventLoopTimer *buttonPollTimer = NULL;

// State variables
static GPIO_Value_Type buttonState = GPIO_Value_High;

static const uint8_t ssd1306Address = 0x3C;

static const uint8_t imageData1[] = {
    #include "image_1.h"
};

static const uint8_t imageData2[] = {
    #include "image_2.h"
};

#define IMAGE_COUNT 2
const void* image[IMAGE_COUNT] = { 0 };
uintptr_t imageSize = 0;
unsigned imageIndex = 0;
bool imageset = 0;
uint8_t remapData[2][sizeof(imageData1)];

// Termination state
static volatile sig_atomic_t exitCode = ExitCode_Success;

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    exitCode = ExitCode_TermHandler_SigTerm;
}

/// <summary>
///     Handle button timer event: if the button is pressed, send data over the UART.
/// </summary>
static void ButtonTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_ButtonTimer_Consume;
        return;
    }

    // Check for a button press
    GPIO_Value_Type newButtonState;
    int result = GPIO_GetValue(gpioButtonFd, &newButtonState);
    if (result != 0) {
        Log_Debug("ERROR: Could not read button GPIO: %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_ButtonTimer_GetValue;
        return;
    }

    // If the button has just been pressed, send data over the UART
    // The button has GPIO_Value_Low when pressed and GPIO_Value_High when released
    if (newButtonState != buttonState) {
        if (newButtonState == GPIO_Value_Low) {
            Log_Debug("Read button GPIO.\n");

            imageset = true;
        }
        buttonState = newButtonState;
    }
}

/// <summary>
///    Checks the number of transferred bytes for I2C functions and prints an error
///    message if the functions failed or if the number of bytes is different than
///    expected number of bytes to be transferred.
/// </summary>
/// <returns>true on success, or false on failure</returns>
static bool CheckTransferSize(const char *desc, size_t expectedBytes, ssize_t actualBytes)
{
    if (actualBytes < 0) {
        Log_Debug("ERROR: %s: errno=%d (%s)\n", desc, errno, strerror(errno));
        return false;
    }

    if (actualBytes != (ssize_t)expectedBytes) {
        Log_Debug("ERROR: %s: transferred %zd bytes; expected %zd\n", desc, actualBytes,
                  expectedBytes);
        return false;
    }

    return true;
}

static void imageRemap(uint8_t* dst, const uint8_t* src)
{
    uint8_t* dp;
    unsigned col;
    for (dp = dst, col = 0; col < SSD1306_WIDTH; col++) {
        unsigned page;
        for (page = 0; page < (SSD1306_HEIGHT / 8); page++, dp++) {
            uint8_t byte = 0;
            unsigned i;
            for (i = 0; i < 8; i++) {
                unsigned y = (page * 8) + i;
                // X inverted to perform bit reversal as PBM is big-endian for bitmaps.
                unsigned xinv = (SSD1306_WIDTH - (col + 1));
                unsigned pixel = (src[((y * SSD1306_WIDTH) + col) / 8] >> (xinv % 8)) & 1;
                // Invert source pixel as PGM stores images inverted.
                byte |= !pixel << i;
            }
            *dp = byte;
        }
    }
}

/// <summary>
///     Set up SIGTERM termination handler, initialize peripherals, and set up event handlers.
/// </summary>
/// <returns>
///     ExitCode_Success if all resources were allocated successfully; otherwise another
///     ExitCode value which indicates the specific failure.
/// </returns>
static ExitCode InitPeripheralsAndHandlers(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = TerminationHandler;
    sigaction(SIGTERM, &action, NULL);

    eventLoop = EventLoop_Create();
    if (eventLoop == NULL) {
        Log_Debug("Could not create event loop.\n");
        return ExitCode_Init_EventLoop;
    }

    // Open USI_MT3620_BT_COMBO_PIN65_GPIO4 GPIO, set as output with value GPIO_Value_Low for using I2C_SDA instead of RXD3
    Log_Debug("Opening USI_MT3620_BT_COMBO_PIN65_GPIO4 as output.\n");
    gpioNSCL = GPIO_OpenAsOutput(USI_MT3620_BT_COMBO_PIN65_GPIO4, GPIO_OutputMode_PushPull, GPIO_Value_Low);
    if (gpioNSCL == -1) {
        Log_Debug("ERROR: Could not open button GPIO: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_NSCL;
    }
    
    i2cFd = I2CMaster_Open(USI_MT3620_BT_COMBO_ISU3_I2C);
    if (i2cFd == -1) {
        Log_Debug("ERROR: I2CMaster_Open: errno=%d (%s)\n", errno, strerror(errno));
        return ExitCode_Init_OpenMaster;
    }

    int result = I2CMaster_SetBusSpeed(i2cFd, I2C_BUS_SPEED_STANDARD);
    if (result != 0) {
        Log_Debug("ERROR: I2CMaster_SetBusSpeed: errno=%d (%s)\n", errno, strerror(errno));
        return ExitCode_Init_SetBusSpeed;
    }

    result = I2CMaster_SetTimeout(i2cFd, 3000);
    if (result != 0) {
        Log_Debug("ERROR: I2CMaster_SetTimeout: errno=%d (%s)\n", errno, strerror(errno));
        return ExitCode_Init_SetTimeout;
    }

    // This default address is used for POSIX read and write calls.  The AppLibs APIs take a target
    // address argument for each read or write.
    result = I2CMaster_SetDefaultTargetAddress(i2cFd, ssd1306Address);
    if (result != 0) {
        Log_Debug("ERROR: I2CMaster_SetDefaultTargetAddress: errno=%d (%s)\n", errno,
                  strerror(errno));
        return ExitCode_Init_SetDefaultTarget;
    }

    bool oledStatus = false;
    oledStatus = Ssd1306_Init(i2cFd);
    if(!oledStatus)
    {
        Log_Debug("Error: OLED initialization failed!\r\n");
    }

    // Remap the image data to match the correct format for the screen
    imageRemap(remapData[0], imageData1);
    imageRemap(remapData[1], imageData2);

    image[0] = remapData[0];
    image[1] = remapData[1];
    imageSize = sizeof(remapData[0]);
    imageIndex = 0;

    if (!SSD1306_WriteFullBuffer(i2cFd, remapData[0], imageSize))
    {
        Log_Debug("Error: SSD1306_WriteFullBuffer failed!\r\n");
    }

    // Open USI_MT3620_BT_COMBO_PIN55_GPIO13 GPIO as input, and set up a timer to poll it.
    Log_Debug("Opening USI_MT3620_BT_COMBO_PIN55_GPIO13 as input.\n");
    gpioButtonFd = GPIO_OpenAsInput(USI_MT3620_BT_COMBO_PIN55_GPIO13);
    if (gpioButtonFd == -1) {
        Log_Debug("ERROR: Could not open button GPIO: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_OpenButton;
    }
    struct timespec buttonPressCheckPeriod1Ms = {.tv_sec = 0, .tv_nsec = 1000 * 1000};
    buttonPollTimer = CreateEventLoopPeriodicTimer(eventLoop, ButtonTimerEventHandler,
                                                   &buttonPressCheckPeriod1Ms);
    if (buttonPollTimer == NULL) {
        return ExitCode_Init_ButtonPollTimer;
    }

    return ExitCode_Success;
}

/// <summary>
///     Closes a file descriptor and prints an error on failure.
/// </summary>
/// <param name="fd">File descriptor to close</param>
/// <param name="fdName">File descriptor name to use in error message</param>
static void CloseFdAndPrintError(int fd, const char *fdName)
{
    if (fd >= 0) {
        int result = close(fd);
        if (result != 0) {
            Log_Debug("ERROR: Could not close fd %s: %s (%d).\n", fdName, strerror(errno), errno);
        }
    }
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    DisposeEventLoopTimer(buttonPollTimer);
    EventLoop_Close(eventLoop);

    Log_Debug("Closing file descriptors.\n");
    CloseFdAndPrintError(gpioButtonFd, "gpioButtonFd");
    CloseFdAndPrintError(i2cFd, "i2c");
    CloseFdAndPrintError(gpioNSCL, "gpioNSCL");
}

/// <summary>
///     Main entry point for this application.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("I2C SSD1306 OLED application starting.\n");
    exitCode = InitPeripheralsAndHandlers();

    // Use event loop to wait for events and trigger handlers, until an error or SIGTERM happens
    while (exitCode == ExitCode_Success) {
        EventLoop_Run_Result result = EventLoop_Run(eventLoop, -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == EventLoop_Run_Failed && errno != EINTR) {
            exitCode = ExitCode_Main_EventLoopFail;
        }

        if (imageset)
		{
			imageIndex = (imageIndex + 1) % IMAGE_COUNT;
            if (!SSD1306_WriteFullBuffer(i2cFd, image[imageIndex], imageSize))
            {
                Log_Debug("Error: SSD1306_WriteFullBuffer failed!\r\n");
            }
			imageset = false;
		}
    }

    ClosePeripheralsAndHandlers();
    Log_Debug("Application exiting.\n");
    return exitCode;
}
