
//
// config.c
// Read the content of a ini file and set the configuration
//
// PiGFX is a bare metal kernel for the Raspberry Pi
// that implements a basic ANSI terminal emulator with
// the additional support of some primitive graphics functions.
// Copyright (C) 2020 Christian Lehner

#include "ee_printf.h"
#include "config.h"

#include "config.h"
#include "emmc.h"
#include "mbr.h"
#include "fat.h"
#include "ee_printf.h"
#include "block.h"
#include "debug_levels.h"
#include "nmalloc.h"
#include "c_utils.h"
#include "ini.h"
#include "gfx.h"
#include "font_registry.h"
#include "framebuffer.h"



int inihandler(void* user, const char* section, const char* name, const char* value)
{
    int tmpValue;

    (void)user;
    (void)section;      // we don't care about the section

    if (pigfx_strcmp(name, "baudrate") == 0)
    {
        tmpValue = atoi(value);
        if (tmpValue > 0) PiGfxConfig.uartBaudrate = tmpValue;
    }
    else if (pigfx_strcmp(name, "useUsbKeyboard") == 0)
    {
        tmpValue = atoi(value);
        if ((tmpValue == 0) || (tmpValue == 1)) PiGfxConfig.useUsbKeyboard = tmpValue;
    }
    else if (pigfx_strcmp(name, "sendCRLF") == 0)
    {
        tmpValue = atoi(value);
        if ((tmpValue == 0) || (tmpValue == 1)) PiGfxConfig.sendCRLF = tmpValue;
    }
    else if (pigfx_strcmp(name, "replaceLFwithCR") == 0)
    {
        tmpValue = atoi(value);
        if ((tmpValue == 0) || (tmpValue == 1)) PiGfxConfig.replaceLFwithCR = tmpValue;
    }
    else if (pigfx_strcmp(name, "backspaceEcho") == 0)
    {
        tmpValue = atoi(value);
        if ((tmpValue == 0) || (tmpValue == 1)) PiGfxConfig.backspaceEcho = tmpValue;
    }
    else if (pigfx_strcmp(name, "skipBackspaceEcho") == 0)
    {
        tmpValue = atoi(value);
        if ((tmpValue == 0) || (tmpValue == 1)) PiGfxConfig.skipBackspaceEcho = tmpValue;
    }
    else if (pigfx_strcmp(name, "swapDelWithBackspace") == 0)
    {
        tmpValue = atoi(value);
        if ((tmpValue == 0) || (tmpValue == 1)) PiGfxConfig.swapDelWithBackspace = tmpValue;
    }
    else if (pigfx_strcmp(name, "keyboardAutorepeat") == 0)
    {
        tmpValue = atoi(value);
        if ((tmpValue == 0) || (tmpValue == 1)) PiGfxConfig.keyboardAutorepeat = tmpValue;
    }
    else if (pigfx_strcmp(name, "keyboardRepeatDelay") == 0)
    {
        tmpValue = atoi(value);
        if (tmpValue > 0) PiGfxConfig.keyboardRepeatDelay = tmpValue;
    }
    else if (pigfx_strcmp(name, "keyboardRepeatRate") == 0)
    {
        tmpValue = atoi(value);
        if (tmpValue > 0) PiGfxConfig.keyboardRepeatRate = tmpValue;
    }
    else if (pigfx_strcmp(name, "foregroundColor") == 0)
    {
        tmpValue = atoi(value);
        if ((tmpValue >= 0) && (tmpValue <= 255)) PiGfxConfig.foregroundColor = tmpValue;
    }
    else if (pigfx_strcmp(name, "backgroundColor") == 0)
    {
        tmpValue = atoi(value);
        if ((tmpValue >= 0) && (tmpValue <= 255)) PiGfxConfig.backgroundColor = tmpValue;
    }
    else if (pigfx_strcmp(name, "fontSelection") == 0)
    {
        tmpValue = atoi(value);
        if (tmpValue >= 0) PiGfxConfig.fontSelection = tmpValue;  // Let font registry validate the range
    }
    else if (pigfx_strcmp(name, "displayWidth") == 0)
    {
        tmpValue = atoi(value);
        if ((tmpValue == 640) || (tmpValue == 1024)) PiGfxConfig.displayWidth = tmpValue;
    }
    else if (pigfx_strcmp(name, "displayHeight") == 0)
    {
        tmpValue = atoi(value);
        if ((tmpValue == 480) || (tmpValue == 768)) PiGfxConfig.displayHeight = tmpValue;
    }
    else if (pigfx_strcmp(name, "disableGfxDMA") == 0)
    {
        tmpValue = atoi(value);
        if ((tmpValue == 0) || (tmpValue == 1)) PiGfxConfig.disableGfxDMA = tmpValue;
    }
    else if (pigfx_strcmp(name, "disableCollision") == 0)
    {
        tmpValue = atoi(value);
        if ((tmpValue == 0) || (tmpValue == 1)) PiGfxConfig.disableCollision = tmpValue;
    }
    else if (pigfx_strcmp(name, "debugVerbosity") == 0)
    {
        tmpValue = atoi(value);
        if ((tmpValue >= 0) && (tmpValue <= 2)) PiGfxConfig.debugVerbosity = tmpValue;
    }
    else if (pigfx_strcmp(name, "cursorBlink") == 0)
    {
        tmpValue = atoi(value);
        if ((tmpValue == 0) || (tmpValue == 1)) PiGfxConfig.cursorBlink = tmpValue;
    }
    else if (pigfx_strcmp(name, "keyboardLayout") == 0)
    {
        pigfx_strncpy(PiGfxConfig.keyboardLayout, value, sizeof(PiGfxConfig.keyboardLayout));
    }
    return 0;
}

void setSafeConfig()
{
    // Set safe/fool-proof configuration for system initialization
    pigfx_memset(&PiGfxConfig, 0, sizeof(PiGfxConfig));

    PiGfxConfig.uartBaudrate = 115200;
    PiGfxConfig.useUsbKeyboard = 1;
    PiGfxConfig.sendCRLF = 0;
    PiGfxConfig.replaceLFwithCR = 0;
    PiGfxConfig.backspaceEcho = 0;
    PiGfxConfig.skipBackspaceEcho = 0;
    PiGfxConfig.swapDelWithBackspace = 1;
    PiGfxConfig.keyboardAutorepeat = 1;
    PiGfxConfig.keyboardRepeatDelay = 500;
    PiGfxConfig.keyboardRepeatRate = 10;
    PiGfxConfig.foregroundColor = 15;    // WHITE (safe foreground)
    PiGfxConfig.backgroundColor = 0;     // BLACK (safe background)
    PiGfxConfig.fontSelection = 0;       // 8x16 System Font (safe font)
    PiGfxConfig.displayWidth = 640;      // Safe resolution: 640x480
    PiGfxConfig.displayHeight = 480;     // Safe resolution: 640x480
    PiGfxConfig.disableGfxDMA = 1;
    PiGfxConfig.disableCollision = 0;
    PiGfxConfig.debugVerbosity = 0;      // Safe default: errors + notices only
    PiGfxConfig.cursorBlink = 1; // Safe default: blinking enabled
    pigfx_strcpy(PiGfxConfig.keyboardLayout, "us");
}

void setDefaultConfig()
{
    // Set default configuration values (fallback if no config file)
    pigfx_memset(&PiGfxConfig, 0, sizeof(PiGfxConfig));

    PiGfxConfig.uartBaudrate = 115200;
    PiGfxConfig.useUsbKeyboard = 1;
    PiGfxConfig.sendCRLF = 0;
    PiGfxConfig.replaceLFwithCR = 0;
    PiGfxConfig.backspaceEcho = 0;
    PiGfxConfig.skipBackspaceEcho = 0;
    PiGfxConfig.swapDelWithBackspace = 1;
    PiGfxConfig.keyboardAutorepeat = 1;  // Enable autorepeat by default
    PiGfxConfig.keyboardRepeatDelay = 500;
    PiGfxConfig.keyboardRepeatRate = 10;
    PiGfxConfig.foregroundColor = 7;     // GRAY (default foreground)
    PiGfxConfig.backgroundColor = 0;     // BLACK (default background)
    PiGfxConfig.fontSelection = 0;       // First font in registry (8x16 System Font)
    PiGfxConfig.displayWidth = 1024;     // Default display width
    PiGfxConfig.displayHeight = 768;     // Default display height
    PiGfxConfig.disableGfxDMA = 1;
    PiGfxConfig.disableCollision = 0;
    PiGfxConfig.debugVerbosity = 0;      // Default: errors + notices only
    PiGfxConfig.cursorBlink = 1; // Default: blinking enabled
    pigfx_strcpy(PiGfxConfig.keyboardLayout, "us");
}

void printLoadedConfig()
{

    if(SHOULD_LOG(LOG_DEBUG_BIT))
    {
    ee_printf("-------------- PiGFX Config Loaded --------------\n");
    ee_printf("uartBaudrate           = %u\n", PiGfxConfig.uartBaudrate);
    ee_printf("useUsbKeyboard         = %u\n", PiGfxConfig.useUsbKeyboard);
    ee_printf("sendCRLF               = %u\n", PiGfxConfig.sendCRLF);
    ee_printf("replaceLFwithCR        = %u\n", PiGfxConfig.replaceLFwithCR);
    ee_printf("backspaceEcho          = %u\n", PiGfxConfig.backspaceEcho);
    ee_printf("skipBackspaceEcho      = %u\n", PiGfxConfig.skipBackspaceEcho);
    ee_printf("swapDelWithBackspace   = %u\n", PiGfxConfig.swapDelWithBackspace);
    ee_printf("keyboardAutorepeat     = %u\n", PiGfxConfig.keyboardAutorepeat);
    ee_printf("keyboardRepeatDelay    = %u\n", PiGfxConfig.keyboardRepeatDelay);
    ee_printf("keyboardRepeatRate     = %u\n", PiGfxConfig.keyboardRepeatRate);
    ee_printf("foregroundColor        = %u\n", PiGfxConfig.foregroundColor);
    ee_printf("backgroundColor        = %u\n", PiGfxConfig.backgroundColor);
    ee_printf("fontSelection          = %u\n", PiGfxConfig.fontSelection);
    ee_printf("displayWidth           = %u\n", PiGfxConfig.displayWidth);
    ee_printf("displayHeight          = %u\n", PiGfxConfig.displayHeight);
    ee_printf("disableGfxDMA          = %u\n", PiGfxConfig.disableGfxDMA);
    ee_printf("disableCollision       = %u\n", PiGfxConfig.disableCollision);
    ee_printf("debugVerbosity         = %u\n", PiGfxConfig.debugVerbosity);
    ee_printf("cursorBlink            = %u\n", PiGfxConfig.cursorBlink);
    ee_printf("keyboardLayout         = %s\n", PiGfxConfig.keyboardLayout);
    ee_printf("-------------------------------------------------\n");
    }
}

unsigned char lookForConfigFile()
{
    int retVal;
    struct block_device *sd_dev = 0;

    if(sd_card_init(&sd_dev) != 0)
    {
        ee_printf("Error initializing SD card\n");
        return errSDCARDINIT;
    }

    if ((read_mbr(sd_dev, (void*)0, (void*)0)) != 0)
    {
        ee_printf("Error reading MasterBootRecord\n");
        return errMBR;
    }

    struct fs * filesys = sd_dev->fs;
    if (filesys == 0)
    {
        ee_printf("Error reading filesystem\n");
        return errFS;
    }

    // loading root dir
    char* myfilename = 0;
    struct dirent *direntry = filesys->read_directory(filesys, &myfilename);
    if (direntry == 0)
    {
        ee_printf("Error reading root directory\n");
        return errREADROOT;
    }

    struct dirent * configfileentry = 0;
    while(1)
    {
        // look for configfile
        if (pigfx_strcmp(CONFIGFILENAME, direntry->name) == 0)
        {
            // File found
            configfileentry = direntry;
            break;
        }
        if (direntry->next) direntry = direntry->next;
        else break;
    }
    if (configfileentry == 0)
    {
        ee_printf("Error locating config file\n");
        return errLOCFILE;
    }

    // read config file
    FILE *configfile = filesys->fopen(filesys, configfileentry, "r");
    if (configfile == 0)
    {
        ee_printf("Error opening config file\n");
        return errOPENFILE;
    }

    ee_printf("Found %s with length %d bytes\n", configfileentry->name, configfile->len);
    char* cfgfiledata = nmalloc_malloc(configfile->len+1);
    cfgfiledata[configfile->len] = 0;       // to be sure that this has a stringend somewhere
    if (filesys->fread(filesys, cfgfiledata, configfile->len, configfile) != (size_t)configfile->len)
    {
        ee_printf("Error reading config file\n");
        nmalloc_free(cfgfiledata);
        return errREADFILE;
    }

    // Interpret file content
    retVal = ini_parse_string(cfgfiledata, inihandler, 0);
    if (retVal < 0)
    {
        ee_printf("Syntax error %d interpreting config file\n", retVal);
        nmalloc_free(cfgfiledata);
        return errSYNTAX;
    }

    nmalloc_free(cfgfiledata);
    // printLoadedConfig();
    return errOK;
}

void applyDisplayConfig()
{
    // Apply current configuration to display system
    // Set colors
    gfx_set_fg(PiGfxConfig.foregroundColor);
    gfx_set_bg(PiGfxConfig.backgroundColor);
    
    // Set font through font registry
    if (font_registry_set_by_index(PiGfxConfig.fontSelection) == 0) {
        // If font index is invalid (returns 0), fall back to default (index 0)
        ee_printf("Warning: Invalid font index %d, using default font\n", PiGfxConfig.fontSelection);
        font_registry_set_by_index(0);
        PiGfxConfig.fontSelection = 0;
    }
}
