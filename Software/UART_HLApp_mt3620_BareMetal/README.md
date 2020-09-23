# Sample: UART_HLApp_mt3620_BareMetal (high-level app)

This sample demonstrates how to communicate over UART on a WIZnet ASG210.

This sample does the following:

- Opens a UART serial port with a baud rate of 115200.
- Sends characters from the device over the UART when user button is pressed.
- Displays the data received from the UART in the Output Window of Visual Studio or Visual Studio Code.
- Sends the data received from the UART back to the UART.

This sample uses these Applibs APIs:

| Library | Purpose |
|---------|---------|
| [UART](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-uart/uart-overview) | Manages UART connectivity on the device |
| [GPIO](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview) | Manages user button on the device |
| TIMER | Manages Poll of input user button |
| [log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Displays messages in the Visual Studio Device Output window during debugging |
| [EventLoop](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invoke handlers for IO and timer events |

## Contents
| File/folder | Description |
|-------------|-------------|
| UART_HLApp_mt3620_BareMetal       |Sample source code and VS project files |
| README.md | This readme file |

## Prerequisites

The sample requires the following hardware:

![Azure Sphere Guardian 210](https://doc.wiznet.io/img/AzureSphere/ASG210_board_description.png)

On RS232/485 connector J7 :

- Connect pins 1 and 3 (ISU3 RXD and ISU3 TXD) to your RS232 TXD, RXD.

## Prepare the sample

1. Ensure that your Azure Sphere device is connected to your computer, and your computer is connected to the internet.
1. Even if you've performed this setup previously, ensure that you have Azure Sphere SDK version 20.07 or above. At the command prompt, run **azsphere show-version** to check. Install the Azure Sphere SDK for [Windows](https://docs.microsoft.com/azure-sphere/install/install-sdk) or [Linux](https://docs.microsoft.com/azure-sphere/install/install-sdk-linux) as needed.
1. Enable application development, if you have not already done so, by entering the following line at the command prompt:

   `azsphere device enable-development`

1. Clone the [ASG210_App](https://github.com/WIZnet-Azure-Sphere/ASG210_App) repo and find the GPIO_HighLevelApp sample in the Software folder.

## Build and run the sample

The application can be run and developed with Visual Studio and Visual Studio Code.

### Run with Visual Studio

Follow these steps to build and run the application with Visual Studio:

1. Start Visual Studio, From the File menu, select Open > Folder… and navigate to the folder, `UART_HLApp_mt3620_BareMetal`.

2. Open app_manifest.json file and check the information correct.

![Visual Studio - Open app_manifest.json](../../Docs/references/visual-studio-open-app-manifest.josn.png)

3. From the Select Startup Item menu, on the tool bar, select GDB Debugger (HLCore).

![Visual Studio - Select GDB Debugger](../../Docs/references/visual-studio-select-gdb-debugger-hl.png)

4. Click Build>Build All to build the project

![Visual Studio - Build the project](../../Docs/references/visual-studio-build-the-project.png)

5. Press <kbd>**F5**</kbd> to start the application with debugging.

### Run with Visual Studio Code

Follow these steps to build and run the application with Visual Studio Code:

1. Open `UART_HLApp_mt3620_BareMetal` folder.

![Visual Studio Code - Open Project Folder](../../Docs/references/visual-studio-code-open-project-folder.png)

2. Press <kbd>**F7**</kbd> to build the project

3. Press <kbd>**F5**</kbd> to start the application with debugging

### Test the sample

1. Press user button on the board. This sends 13 bytes over the UART connection and displays the sent and received text in the Device Output window, if you're using Visual Studio or Visual Studio Code:

   `Sent 13 bytes over UART in 1 calls`  
   `UART received 12 bytes: 'Hello world!'`  
   `UART received 1 bytes: '`  
   `'`

   Sends the data received from the UART back to the UART.

   All the received text might not appear at once, and it might not appear immediately. 

