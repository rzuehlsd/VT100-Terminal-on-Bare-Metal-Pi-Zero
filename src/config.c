
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



// Helper functions for configuration parsing
/**
 * @brief Set boolean configuration value (0 or 1)
 * 
 * Validates that the string value represents a boolean (0 or 1) and sets
 * the configuration field if valid. Invalid values are ignored.
 * 
 * @param name The configuration parameter name (for debugging)
 * @param value The string value to parse and validate
 * @param config_field Pointer to the configuration field to update
 */
static void set_boolean_config(const char* name, const char* value, unsigned int* config_field)
{
    (void)name; // Suppress unused parameter warning
    int tmpValue = atoi(value);
    if ((tmpValue == 0) || (tmpValue == 1)) {
        *config_field = tmpValue;
    }
}

/**
 * @brief Set positive integer configuration value
 * 
 * Validates that the string value represents a positive integer (> 0) and sets
 * the configuration field if valid. Zero and negative values are ignored.
 * 
 * @param name The configuration parameter name (for debugging)
 * @param value The string value to parse and validate
 * @param config_field Pointer to the configuration field to update
 */
static void set_positive_config(const char* name, const char* value, unsigned int* config_field)
{
    (void)name; // Suppress unused parameter warning
    int tmpValue = atoi(value);
    if (tmpValue > 0) {
        *config_field = tmpValue;
    }
}

/**
 * @brief Set configuration value within specified range
 * 
 * Validates that the string value represents an integer within the specified
 * range [min_val, max_val] and sets the configuration field if valid.
 * Values outside the range are ignored.
 * 
 * @param name The configuration parameter name (for debugging)
 * @param value The string value to parse and validate
 * @param config_field Pointer to the configuration field to update
 * @param min_val Minimum allowed value (inclusive)
 * @param max_val Maximum allowed value (inclusive)
 */
static void set_range_config(const char* name, const char* value, unsigned int* config_field, int min_val, int max_val)
{
    (void)name; // Suppress unused parameter warning
    int tmpValue = atoi(value);
    if ((tmpValue >= min_val) && (tmpValue <= max_val)) {
        *config_field = tmpValue;
    }
}

/**
 * @brief Set configuration value from list of allowed values
 * 
 * Validates that the string value represents an integer that matches one of
 * the values in the provided valid_values array. Sets the configuration field
 * if a match is found, otherwise ignores the value.
 * 
 * @param name The configuration parameter name (for debugging)
 * @param value The string value to parse and validate
 * @param config_field Pointer to the configuration field to update
 * @param valid_values Array of allowed integer values
 * @param count Number of elements in the valid_values array
 */
static void set_specific_values_config(const char* name, const char* value, unsigned int* config_field, const int* valid_values, int count)
{
    (void)name; // Suppress unused parameter warning
    int tmpValue = atoi(value);
    for (int i = 0; i < count; i++) {
        if (tmpValue == valid_values[i]) {
            *config_field = tmpValue;
            break;
        }
    }
}
    
/**
 * @brief INI file parser callback handler
 * 
 * This function is called by the INI parser for each key-value pair found
 * in the configuration file. It maps configuration parameter names to their
 * corresponding fields in the PiVT100Config structure and applies appropriate
 * validation for each parameter type.
 * 
 * Supported configuration parameters:
 * - baudrate: UART baud rate (positive integer)
 * - useUsbKeyboard: Enable USB keyboard (0/1)
 * - sendCRLF, replaceLFwithCR, backspaceEcho, etc.: Various boolean flags
 * - keyboardRepeatDelay, keyboardRepeatRate: Positive integers
 * - foregroundColor, backgroundColor: Color values (0-255)
 * - displayWidth, displayHeight: Specific allowed display dimensions
 * - debugVerbosity: Debug level (0-2)
 * - keyboardLayout: String value (copied directly)
 * 
 * @param user User data pointer (unused)
 * @param section INI section name (unused - we don't use sections)
 * @param name Configuration parameter name
 * @param value Configuration parameter value as string
 * @return Always returns 0 (continue parsing)
 * 
 * @note Sets PiVT100Config.hasChanged = 1 after any parameter is processed
 * @note Invalid values for any parameter are silently ignored
 */
int inihandler(void* user, const char* section, const char* name, const char* value)
{
    (void)user;
    (void)section;      // we don't care about the section

    if (pivt100_strcmp(name, "baudrate") == 0)
    {
        set_positive_config(name, value, &PiVT100Config.uartBaudrate);
    }
    else if (pivt100_strcmp(name, "switchRxTx") == 0)
    {
        set_boolean_config(name, value, &PiVT100Config.switchRxTx);
    }
    else if (pivt100_strcmp(name, "useUsbKeyboard") == 0)
    {
        set_boolean_config(name, value, &PiVT100Config.useUsbKeyboard);
    }
    else if (pivt100_strcmp(name, "sendCRLF") == 0)
    {
        set_boolean_config(name, value, &PiVT100Config.sendCRLF);
    }
    else if (pivt100_strcmp(name, "replaceLFwithCR") == 0)
    {
        set_boolean_config(name, value, &PiVT100Config.replaceLFwithCR);
    }
    else if (pivt100_strcmp(name, "backspaceEcho") == 0)
    {
        set_boolean_config(name, value, &PiVT100Config.backspaceEcho);
    }
    else if (pivt100_strcmp(name, "skipBackspaceEcho") == 0)
    {
        set_boolean_config(name, value, &PiVT100Config.skipBackspaceEcho);
    }
    else if (pivt100_strcmp(name, "swapDelWithBackspace") == 0)
    {
        set_boolean_config(name, value, &PiVT100Config.swapDelWithBackspace);
    }
    else if (pivt100_strcmp(name, "keyboardAutorepeat") == 0)
    {
        set_boolean_config(name, value, &PiVT100Config.keyboardAutorepeat);
    }
    else if (pivt100_strcmp(name, "keyboardRepeatDelay") == 0)
    {
        set_positive_config(name, value, &PiVT100Config.keyboardRepeatDelay);
    }
    else if (pivt100_strcmp(name, "keyboardRepeatRate") == 0)
    {
        set_positive_config(name, value, &PiVT100Config.keyboardRepeatRate);
    }
    else if (pivt100_strcmp(name, "foregroundColor") == 0)
    {
        set_range_config(name, value, &PiVT100Config.foregroundColor, 0, 255);
    }
    else if (pivt100_strcmp(name, "backgroundColor") == 0)
    {
        set_range_config(name, value, &PiVT100Config.backgroundColor, 0, 255);
    }
    else if (pivt100_strcmp(name, "fontSelection") == 0)
    {
        int tmpValue = atoi(value);
        if (tmpValue >= 0) PiVT100Config.fontSelection = tmpValue;  // Let font registry validate the range
    }
    else if (pivt100_strcmp(name, "displayWidth") == 0)
    {
        static const int valid_widths[] = {640, 800, 1024};
        set_specific_values_config(name, value, &PiVT100Config.displayWidth, valid_widths, 3);
    }
    else if (pivt100_strcmp(name, "displayHeight") == 0)
    {
        static const int valid_heights[] = {480, 640, 768};
        set_specific_values_config(name, value, &PiVT100Config.displayHeight, valid_heights, 3);
    }
    else if (pivt100_strcmp(name, "disableGfxDMA") == 0)
    {
        set_boolean_config(name, value, &PiVT100Config.disableGfxDMA);
    }
    // disableCollision removed (sprite system no longer present)
    else if (pivt100_strcmp(name, "debugVerbosity") == 0)
    {
        set_range_config(name, value, &PiVT100Config.debugVerbosity, 0, 2);
    }
    else if (pivt100_strcmp(name, "cursorBlink") == 0)
    {
        set_boolean_config(name, value, &PiVT100Config.cursorBlink);
    }
    else if (pivt100_strcmp(name, "soundLevel") == 0)
    {
        set_range_config(name, value, &PiVT100Config.soundLevel, 0, 100);
    }
    else if (pivt100_strcmp(name, "keyClick") == 0)
    {
        set_boolean_config(name, value, &PiVT100Config.keyClick);
    }
    else if (pivt100_strcmp(name, "keyboardLayout") == 0)
    {
        pivt100_strncpy(PiVT100Config.keyboardLayout, value, sizeof(PiVT100Config.keyboardLayout));
    }

    PiVT100Config.hasChanged = 1;
    return 0;
}



/**
 * @brief Initialize configuration with default values
 * 
 * Sets all configuration parameters in PiVT100Config to their default values.
 * This function is called during system initialization or when no configuration
 * file is found on the SD card.
 * 
 * Default values include:
 * - UART baud rate: 115200
 * - USB keyboard: enabled
 * - Display: 800x640 pixels
 * - Font: first in registry (8x16 System Font)
 * - Colors: gray foreground on black background
 * - Keyboard layout: German ("de")
 * - Debug verbosity: errors and notices only
 * 
 * @note Sets PiVT100Config.hasChanged = 1 to trigger configuration application
 * @note Clears the entire structure before setting values
 */
void setDefaultConfig()
{
    // Set default configuration values (fallback if no config file)
    pivt100_memset(&PiVT100Config, 0, sizeof(PiVT100Config));
    PiVT100Config.hasChanged = 1;
    PiVT100Config.uartBaudrate = 115200;
    PiVT100Config.useUsbKeyboard = 1;
    PiVT100Config.sendCRLF = 0;
    PiVT100Config.replaceLFwithCR = 1;
    PiVT100Config.backspaceEcho = 0;
    PiVT100Config.skipBackspaceEcho = 0;
    PiVT100Config.swapDelWithBackspace = 1;
    PiVT100Config.keyboardAutorepeat = 1;  // Enable autorepeat by default
    PiVT100Config.keyboardRepeatDelay = 500;
    PiVT100Config.keyboardRepeatRate = 10;
    PiVT100Config.foregroundColor = 11;     // Yellow (default foreground)
    PiVT100Config.backgroundColor = 0;     // BLACK (default background)
    PiVT100Config.fontSelection = 2;       // First font in registry (8x16 System Font)
    PiVT100Config.displayWidth = 1024;     // Default display width
    PiVT100Config.displayHeight = 768;     // Default display height
    PiVT100Config.disableGfxDMA = 1;
    // disableCollision removed
    PiVT100Config.debugVerbosity = 2;     // Default: all debug levels enabled
    PiVT100Config.cursorBlink = 0;            // Default: blinking disabled
    PiVT100Config.switchRxTx = 0;          // Default: normal UART operation
    PiVT100Config.soundLevel = 50;         // Default sound level (duty %) for beep
    PiVT100Config.keyClick = 1;            // Default: keyclick enabled
    pivt100_strcpy(PiVT100Config.keyboardLayout, "de");
}

/**
 * @brief Print current configuration values to debug output
 * 
 * Outputs all configuration parameters and their current values in a formatted
 * table for debugging and verification purposes. This function is typically
 * called after configuration loading to confirm the active settings.
 * 
 * The output includes:
 * - All UART and keyboard settings
 * - Display dimensions and graphics options
 * - Color and font selections
 * - Debug and timing parameters
 * 
 * @note Uses ee_printf() for output to the debug console
 * @note Safe to call at any time after PiVT100Config is initialized
 */
void printConfig()
{
    LogDebug("-------------- PiGFX Config Loaded --------------\n");
    LogDebug("hasChanged.            = %u\n", PiVT100Config.hasChanged);
    LogDebug("uartBaudrate           = %u\n", PiVT100Config.uartBaudrate);
    LogDebug("switchRxTx             = %u\n", PiVT100Config.switchRxTx);
    LogDebug("useUsbKeyboard         = %u\n", PiVT100Config.useUsbKeyboard);
    LogDebug("sendCRLF               = %u\n", PiVT100Config.sendCRLF);
    LogDebug("replaceLFwithCR        = %u\n", PiVT100Config.replaceLFwithCR);
    LogDebug("backspaceEcho          = %u\n", PiVT100Config.backspaceEcho);
    LogDebug("skipBackspaceEcho      = %u\n", PiVT100Config.skipBackspaceEcho);
    LogDebug("swapDelWithBackspace   = %u\n", PiVT100Config.swapDelWithBackspace);
    LogDebug("keyboardAutorepeat     = %u\n", PiVT100Config.keyboardAutorepeat);
    LogDebug("keyboardRepeatDelay    = %u\n", PiVT100Config.keyboardRepeatDelay);
    LogDebug("keyboardRepeatRate     = %u\n", PiVT100Config.keyboardRepeatRate);
    LogDebug("foregroundColor        = %u\n", PiVT100Config.foregroundColor);
    LogDebug("backgroundColor        = %u\n", PiVT100Config.backgroundColor);
    LogDebug("fontSelection          = %u\n", PiVT100Config.fontSelection);
    LogDebug("displayWidth           = %u\n", PiVT100Config.displayWidth);
    LogDebug("displayHeight          = %u\n", PiVT100Config.displayHeight);
    LogDebug("disableGfxDMA          = %u\n", PiVT100Config.disableGfxDMA);
    // disableCollision removed
    LogDebug("debugVerbosity         = %u\n", PiVT100Config.debugVerbosity);
    LogDebug("cursorBlink            = %u\n", PiVT100Config.cursorBlink);
    LogDebug("soundLevel             = %u\n", PiVT100Config.soundLevel);
    LogDebug("keyClick               = %u\n", PiVT100Config.keyClick);
    LogDebug("keyboardLayout         = %s\n", PiVT100Config.keyboardLayout);
    LogDebug("-------------------------------------------------\n");
}

/**
 * @brief Load configuration from pivt100.txt file on SD card
 * 
 * Attempts to read and parse the configuration file "pivt100.txt" from the root
 * directory of the SD card. The file uses INI format with key=value pairs.
 * 
 * The function performs the following steps:
 * 1. Initialize SD card interface
 * 2. Read Master Boot Record (MBR)
 * 3. Mount filesystem (typically FAT32)
 * 4. Search for pivt100.txt in root directory
 * 5. Parse the file using INI parser with inihandler callback
 * 6. Clean up allocated resources
 * 
 * @return Error code indicating success or failure:
 *         - errOK: Configuration loaded successfully
 *         - errSDCARDINIT: SD card initialization failed
 *         - errMBR: Master Boot Record read error
 *         - errFS: Filesystem mount error
 *         - errREADROOT: Root directory read error
 *         - errLOCFILE: Configuration file not found
 *         - errOPENFILE: Cannot open configuration file
 *         - errREADFILE: File read error
 *         - errSYNTAX: INI parsing error
 * 
 * @note If file is not found or any error occurs, default configuration should be used
 * @note The function automatically handles memory allocation/deallocation for file operations
 */
unsigned char loadConfigFile()
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
        // look for configfile (case-insensitive on FAT)
        const char* want = CONFIGFILENAME;
        const char* have = direntry->name;
        // Manual case-insensitive compare to avoid locale issues
        unsigned ci_equal = 1;
        while (*want || *have)
        {
            char a = *want++;
            char b = *have++;
            if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
            if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
            if (a != b) { ci_equal = 0; break; }
        }
        if (ci_equal)
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

/**
 * @brief Convert debug verbosity level to debug severity bitmask
 * 
 * Converts the user-friendly debug verbosity level (0-2) from configuration
 * into the internal debug severity bitmask used by the logging system.
 * 
 * Debug levels:
 * - Level 0: Errors and notices only
 * - Level 1: + Warnings
 * - Level 2: + Debug messages (all messages)
 * 
 * @param level Debug verbosity level (0-2)
 * @return Debug severity bitmask for use with SetDebugSeverity()
 * 
 * @note Higher levels include all lower level messages
 * @note Invalid levels default to level 0 behavior
 */
unsigned int debugLevel(int level)
{
    unsigned int debugSeverity = LOG_ERROR_BIT | LOG_NOTICE_BIT;
    if (level >= 1) {
        debugSeverity |= LOG_WARNING_BIT;
    }
    if (level >= 2) {
        debugSeverity |= LOG_DEBUG_BIT;
    }
    return debugSeverity;
}

/**
 * @brief Apply configuration changes to hardware and system settings
 * 
 * Takes the current configuration values in PiVT100Config and applies them to
 * the actual hardware and software subsystems. This function should be called
 * after loading configuration or when settings have been modified.
 * 
 * The function applies changes to:
 * - Display framebuffer (resolution and color depth)
 * - Graphics drawing mode and cursor settings
 * - Foreground and background colors
 * - Font selection through font registry
 * - UART baud rate and interrupt configuration
 * - Debug verbosity level
 * 
 * Optimization: If PiVT100Config.hasChanged is 0, the function returns early
 * without making any changes, avoiding unnecessary reinitialization.
 * 
 * @note Clears the screen after applying new configuration
 * @note Resets PiVT100Config.hasChanged to 0 after successful application
 * @note Safe to call multiple times - only applies changes when needed
 */

 extern void initialize_uart_irq();
 extern void initialize_framebuffer(unsigned int width, unsigned int height, unsigned int bpp);
 extern void uart_init(unsigned int baudrate);

 void applyConfig()
{
    // Apply current configuration to display system
    if(PiVT100Config.hasChanged == 0) return;  // No change, nothing to do
        PiVT100Config.hasChanged = 0;

    // Reinitialize framebuffer if display size changed
    initialize_framebuffer(PiVT100Config.displayWidth, PiVT100Config.displayHeight, 8);         

    // Set drawing mode, cusor and colors
    gfx_set_drawing_mode(drawingNORMAL);
    gfx_term_set_cursor_blinking(PiVT100Config.cursorBlink);

    gfx_set_fg(PiVT100Config.foregroundColor);
    gfx_set_bg(PiVT100Config.backgroundColor);
    
    // Set font through font registry
    gfx_term_set_font(PiVT100Config.fontSelection);

    // Set tabu stops      // has to be included in setup dialog
    gfx_term_set_tabulation(8);   

    // Reinitialize UART with new baudrate
    uart_init(PiVT100Config.uartBaudrate);

    // Apply debug verbosity setting from configuration immediately
    // 0 = errors + notices, 1 = +warnings, 2 = +debug
    SetDebugSeverity(debugLevel(PiVT100Config.debugVerbosity));

}
