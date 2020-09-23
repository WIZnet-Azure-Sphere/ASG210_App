/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#include "SSD1306.h"

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned reserved : 6;
        bool     isData   : 1;
        bool     cont     : 1;
    };
    uint8_t mask;
} Ssd1306_I2CHeader;

typedef struct __attribute__((__packed__)) {
    Ssd1306_I2CHeader header;
    uint8_t           data;
} Ssd1306_Packet;

#define SSD1306_MAX_DATA_WRITE 1024
static bool Ssd1306_Write(int fd, bool isData, const void *data, uintptr_t size)
{
    Ssd1306_Packet cmd[SSD1306_MAX_DATA_WRITE];

    if (size > SSD1306_MAX_DATA_WRITE) {
        return false;
    }

    uintptr_t i;
    for (i = 0; i < size; i++) {
        cmd[i].header = (Ssd1306_I2CHeader){ .isData = isData, .cont = (i < (size - 1)) };
        cmd[i].data   = ((uint8_t *)data)[i];
    }
    
    ssize_t transferredBytes;
    transferredBytes = I2CMaster_Write(fd, SSD1306_ADDRESS, cmd, (size * sizeof(cmd[0])));

    if (transferredBytes != (size * sizeof(cmd[0])))
    {
        Log_Debug("transferredBytes 0x%x\r\n", transferredBytes);
        Log_Debug("size * sizeof(cmd[0]) 0x%x\r\n", size * sizeof(cmd[0]));
        return false;
    }

    return true;
}

//Static functions for hardware configuration
static bool SSD1306_SetDisplayClockDiv(int fd, uint8_t value)
{
    uint8_t packet[] = { SSD1306_CMD_SETDISPLAYCLOCKDIV, value };
    return Ssd1306_Write(fd, false, packet, sizeof(packet));
}

static bool SSD1306_SetMultiplex(int fd, uint8_t value)
{
    uint8_t packet[] = { SSD1306_CMD_SETMULTIPLEX, value };
    return Ssd1306_Write(fd, false, packet, sizeof(packet));
}

static bool SSD1306_SetDisplayOffset(int fd, uint8_t value)
{
    uint8_t packet[] = { SSD1306_CMD_SETDISPLAYOFFSET, value };
    return Ssd1306_Write(fd, false, packet, sizeof(packet));
}


static bool SSD1306_SetChargePump(int fd, uint8_t value)
{
    uint8_t packet[] = { SSD1306_CMD_CHARGEPUMP, value };
    return Ssd1306_Write(fd, false, packet, sizeof(packet));
}

static bool SSD1306_SetStartLine(int fd, unsigned offset)
{
    if (offset >= SSD1306_HEIGHT) {
        return false;
    }

    uint8_t value = SSD1306_CMD_SETSTARTLINE + offset;
    return Ssd1306_Write(fd, false, &value, sizeof(value));
}

static bool SSD1306_SetMemoryMode(int fd, uint8_t value)
{
    uint8_t packet[] = { SSD1306_CMD_MEMORYMODE, value };
    return Ssd1306_Write(fd, false, packet, sizeof(packet));
}

static bool SSD1306_SetSegRemap(int fd, bool remapTrue)
{
    uint8_t value = SSD1306_CMD_SETSEGMENTREMAP | remapTrue;
    return Ssd1306_Write(fd, false, &value, sizeof(value));
}

static bool SSD1306_SetComScanDir(int fd, bool scanDirTrue)
{
    uint8_t value = SSD1306_CMD_COMSCANDIR | (scanDirTrue ? 8 : 0);
    return Ssd1306_Write(fd, false, &value, sizeof(value));
}

static bool SSD1306_SetComPins(int fd, uint8_t value)
{
    uint8_t packet[] = { SSD1306_CMD_SETCOMPINS, value };
    return Ssd1306_Write(fd, false, packet, sizeof(packet));
}

static bool SSD1306_SetPreCharge(int fd, uint8_t value)
{
    uint8_t packet[] = { SSD1306_CMD_SETPRECHARGE, value };
    return Ssd1306_Write(fd, false, packet, sizeof(packet));
}

static bool SSD1306_SetVComDetect(int fd, uint8_t value)
{
    uint8_t packet[] = { SSD1306_CMD_SETVCOMDETECT, value };
    return Ssd1306_Write(fd, false, packet, sizeof(packet));
}
//End of hardware configuration functions

//Sets the start and end column address for the data that is being sent
static bool SSD1306_SetColumnAddress(int fd, uint8_t columnStart, uint8_t columnEnd)
{
    if ((columnStart >= SSD1306_WIDTH) || (columnEnd >= SSD1306_WIDTH)) {
        return false;
    }
    uint8_t packet[] = { SSD1306_CMD_SETCOLUMNADDR, columnStart, columnEnd };
    return Ssd1306_Write(fd, false, packet, sizeof(packet));
}

static bool SSD1306_SetPageAddress(int fd, uint8_t pageStart, uint8_t pageEnd)
{
    if ((pageStart >= SSD1306_HEIGHT) || (pageEnd >= SSD1306_HEIGHT)) {
        return false;
    }
    uint8_t packet[] = { SSD1306_CMD_SETPAGEADDR, pageStart, pageEnd };
    return Ssd1306_Write(fd, false, packet, sizeof(packet));
}

bool SSD1306_WriteFullBuffer(int fd, const void *data, uintptr_t size)
{
    if (!SSD1306_SetColumnAddress(fd, 0, (SSD1306_WIDTH - 1))) {
        return false;
    }

    if (!SSD1306_SetPageAddress(fd, 0, (SSD1306_HEIGHT - 1))) {
        return false;
    }

    return Ssd1306_Write(fd, true, data, size);
}

bool SSD1306_SetDisplayOnOff(int fd, bool displayOnTrue)
{
    uint8_t value = SSD1306_CMD_DISPLAYONOFF | displayOnTrue;
    return Ssd1306_Write(fd, false, &value, sizeof(value));
}

bool SSD1306_SetContrast(int fd, uint8_t value)
{
    uint8_t packet[] = { SSD1306_CMD_SETCONTRAST, value };
    return Ssd1306_Write(fd, false, packet, sizeof(packet));
}

bool SSD1306_SetDisplayAllOn(int fd, bool displayAllOnTrue)
{
    int8_t value = SSD1306_CMD_DISPLAYALLON | displayAllOnTrue;
    return Ssd1306_Write(fd, false, &value, sizeof(value));
}

bool SSD1306_SetDisplayInverse(int fd, bool inverseTrue)
{
    uint8_t value = SSD1306_CMD_NORMALDISPLAY | inverseTrue;
    return Ssd1306_Write(fd, false, &value, sizeof(value));
}

bool SSD1306_ActivateScroll(int fd, bool activateScrollTrue)
{
    uint8_t value = SSD1306_CMD_SETSCROLL | activateScrollTrue;
    return Ssd1306_Write(fd, false, &value, sizeof(value));
}

bool Ssd1306_Init(int fd)
{
    bool success = true;

    success = success && SSD1306_SetDisplayOnOff(fd, false);
    success = success && SSD1306_SetDisplayClockDiv(fd, SSD1306_CTRL_DISPLAYCLOCKDIV);
    success = success && SSD1306_SetMultiplex(fd, SSD1306_CTRL_MULTIPLEX);
    success = success && SSD1306_SetDisplayOffset(fd, SSD1306_CTRL_DISPLAYOFFSET);
    success = success && SSD1306_SetStartLine(fd, SSD1306_CTRL_STARTLINE);
    success = success && SSD1306_SetChargePump(fd, SSD1306_CTRL_CHARGEPUMP);
    success = success && SSD1306_SetMemoryMode(fd, SSD1306_CTRL_MEMORYMODE);
    success = success && SSD1306_SetSegRemap(fd, SSD1306_CTRL_SEGREMAP);
    success = success && SSD1306_SetComScanDir(fd, SSD1306_CTRL_COMSCANDIR);
    success = success && SSD1306_SetComPins(fd, SSD1306_CTRL_COMPIMS);
    success = success && SSD1306_SetContrast(fd, SSD1306_CTRL_CONTRAST);
    success = success && SSD1306_SetPreCharge(fd, SSD1306_CTRL_PRECHARGE);
    success = success && SSD1306_SetVComDetect(fd, SSD1306_CTRL_VCOMDETECT);
    success = success && SSD1306_SetDisplayAllOn(fd, false);
    success = success && SSD1306_SetDisplayInverse(fd, SSD1306_CTRL_DISPLAYINVERSE);
    success = success && SSD1306_ActivateScroll(fd, SSD1306_CTRL_ACTIVATESCROLL);
    success = success && SSD1306_SetDisplayOnOff(fd, true);

    return success;
}

