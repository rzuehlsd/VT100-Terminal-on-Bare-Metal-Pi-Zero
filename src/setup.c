//
// setup.c
// Setup mode functions for PiGFX configuration
//
// PiGFX is a bare metal kernel for the Raspberry Pi
// that implements a basic ANSI terminal emulator with
// the additional support of some primitive graphics functions.
// Copyright (C) 2025

#include "setup.h"
#include "gfx.h"
#include "gfx_types.h"
#include "config.h"
#include "ee_printf.h"
#include "nmalloc.h"
#include "keyboard.h"
#include "uart.h"
#include "font_registry.h"
#include <stddef.h>
#include "myString.h"
#include <string.h>
#include "gfx.h"

// Forward declaration for initialize_framebuffer
extern void initialize_framebuffer(unsigned int width, unsigned int height, unsigned int bpp);
// Apply UART pin switch immediately after saving from setup
extern void switch_uart_pins(void);

static unsigned char setup_mode_active = 0;
static void* saved_screen_buffer = 0;
static unsigned char saved_cursor_visibility = 0;
static GFX_COL saved_fg_color = 0;
static GFX_COL saved_bg_color = 0;
static int saved_font_type = 0;  // Use font type instead of width/height
static unsigned char needs_redraw = 1;  // Flag to control when to redraw
static unsigned char settings_changed = 0;  // Flag to track if user made any changes
static unsigned int original_font_index = 0;  // Track original font when entering setup

// Setup menu state
static unsigned int selected_item = 0;  // 0 = Baudrate, 1 = Keyboard, 2 = Foreground, 3 = Background, 4 = Font, 5 = Resolution, 6 = Cursor Blink, 7 = Repeat Delay, 8 = Repeat Rate
static unsigned int selected_baudrate_index = 0;
static unsigned int selected_switch_rxtx = 0; // New: toggle Rx<>Tx switch
static unsigned int selected_keyboard_index = 0;
static unsigned int selected_fg_color = 0;
static unsigned int selected_bg_color = 0;
static unsigned int selected_font_size = 0;
static unsigned int selected_resolution_index = 0;
static unsigned int selected_cursor_blink = 1;
static unsigned int selected_auto_repeat = 1;  // Default auto repeat
static unsigned int selected_send_crlf = 0;    // Default CRLF sending toggle
static unsigned int selected_replace_lf_cr = 0; // New: Replace LF with CR toggle
static unsigned int selected_repeat_delay = 500;
static unsigned int selected_repeat_rate = 10;
static const unsigned int num_setup_items = 15;  // Number of setup items (added Switch Rx<>Tx, Send CRLF, Replace LF->CR, Sound Level, Key Click)

// Sound level variable
static unsigned int selected_sound_level = 0;  // Sound level (duty %) 0..100

// Key click variable
static unsigned int selected_key_click = 1;    // Key click enabled/disabled

// Available baudrates
static const unsigned int available_baudrates[] = {
    300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200
};
static const unsigned int num_baudrates = sizeof(available_baudrates) / sizeof(available_baudrates[0]);

// Available keyboard layouts
static const char* available_keyboards[] = {
    "us", "uk", "it", "fr", "es", "de", "sg"
};
static const unsigned int num_keyboards = sizeof(available_keyboards) / sizeof(available_keyboards[0]);

// Available colors for foreground/background - reduced to 4 colors only
static const GFX_COL available_colors[] = {
    BLACK, GREEN, YELLOW, WHITE
};
static const char* color_names[] = {
    "Black", "Green", "Yellow", "White"
};
static const unsigned int num_colors = sizeof(available_colors) / sizeof(available_colors[0]);

// Available resolutions
static const char* available_resolutions[] = {
    "640x480", "800x640", "1024x768"
};
static const unsigned int resolution_widths[] = {
    640, 800, 1024
};
static const unsigned int resolution_heights[] = {
    480, 640, 768
};
static const unsigned int num_resolutions = sizeof(available_resolutions) / sizeof(available_resolutions[0]);

// Font switching function that uses font registry
/**
 * @brief Switch to a specific font by registry index
 * 
 * Changes the current font to the one specified by the font registry index.
 * Used during setup mode to preview different fonts.
 * 
 * @param font_index Index of the font in the font registry
 */
static void switch_to_font_by_index(int font_index)
{
    gfx_term_set_font(font_index);
}

// Helper function to draw text at specific position without affecting cursor
/**
 * @brief Draw text at specified terminal position
 * 
 * Renders text at the given row and column position using terminal coordinates.
 * Uses the current foreground and background colors.
 * 
 * @param row Terminal row position (0-based)
 * @param col Terminal column position (0-based)
 * @param text Null-terminated string to display
 */
static void draw_text_at(unsigned int row, unsigned int col, const char* text)
{
    const char* p = text;
    unsigned int current_col = col;
    while (*p)
    {
        gfx_putc(row, current_col, *p);
        p++;
        current_col++;
    }
}

// Helper function to draw text with background clearing
/**
 * @brief Draw text with background clearing to specified length
 * 
 * Renders text at the given position and clears the background to the specified
 * length with space characters. This ensures clean display when overwriting
 * existing text with shorter text.
 * 
 * @param row Terminal row position (0-based)
 * @param col Terminal column position (0-based)
 * @param text Null-terminated string to display
 * @param length Total length to clear (text + trailing spaces)
 */
static void draw_text_at_with_bg(unsigned int row, unsigned int col, const char* text, unsigned int length)
{
    const char* p = text;
    unsigned int current_col = col;
    unsigned int i = 0;
    
    // Draw the text
    while (*p && i < length)
    {
        gfx_putc(row, current_col, *p);
        p++;
        current_col++;
        i++;
    }
    
    // Fill remaining space with spaces to clear background
    while (i < length)
    {
        gfx_putc(row, current_col, ' ');
        current_col++;
        i++;
    }
}

// Helper function to draw integer at specific position
/**
 * @brief Draw an unsigned integer at specified terminal position
 * 
 * @param row Terminal row position (0-based)
 * @param col Terminal column position (0-based) 
 * @param value Unsigned integer value to display
 */
static void __attribute__((unused)) draw_int_at(unsigned int row, unsigned int col, unsigned int value)
{
    char buffer[16];
    // Simple integer to string conversion
    char* p = buffer + sizeof(buffer) - 1;
    *p = '\0';
    
    if (value == 0)
    {
        *(--p) = '0';
    }
    else
    {
        while (value > 0)
        {
            *(--p) = '0' + (value % 10);
            value /= 10;
        }
    }
    
    draw_text_at(row, col, p);
}

// Helper function to draw integer with background clearing
/**
 * @brief Draw integer value with background clearing
 * 
 * Converts an unsigned integer to string and displays it with background
 * clearing to the specified length. This prevents visual artifacts when
 * overwriting longer numbers with shorter ones.
 * 
 * @param row Terminal row position (0-based)
 * @param col Terminal column position (0-based)
 * @param value Unsigned integer value to display
 * @param length Total field length to clear with spaces
 */
static void __attribute__((unused)) draw_int_at_with_bg(unsigned int row, unsigned int col, unsigned int value, unsigned int length)
{
    char buffer[16];
    // Simple integer to string conversion
    char* p = buffer + sizeof(buffer) - 1;
    *p = '\0';
    
    if (value == 0)
    {
        *(--p) = '0';
    }
    else
    {
        while (value > 0)
        {
            *(--p) = '0' + (value % 10);
            value /= 10;
        }
    }
    
    draw_text_at_with_bg(row, col, p, length);
}

// Find current baudrate index in available_baudrates array
/**
 * @brief Find index of current baud rate in available options
 * 
 * Searches the available_baudrates array to find the index that matches
 * the current UART baud rate configuration.
 * 
 * @return Index in available_baudrates array, or 0 if not found
 */
static unsigned int find_current_baudrate_index(void)
{
    unsigned int current_baudrate = PiGfxConfig.uartBaudrate;
    for (unsigned int i = 0; i < num_baudrates; i++)
    {
        if (available_baudrates[i] == current_baudrate)
        {
            return i;
        }
    }
    // Default to 115200 if current baudrate not found
    return num_baudrates - 1;
}

// Find current keyboard layout index in available_keyboards array
/**
 * @brief Find index of current keyboard layout in available options
 * 
 * Searches the available_keyboards array to find the index that matches
 * the current keyboard layout configuration.
 * 
 * @return Index in available_keyboards array, or 0 if not found
 */
static unsigned int find_current_keyboard_index(void)
{
    for (unsigned int i = 0; i < num_keyboards; i++)
    {
        if (PiGfxConfig.keyboardLayout[0] == available_keyboards[i][0] && 
            PiGfxConfig.keyboardLayout[1] == available_keyboards[i][1])
        {
            return i;
        }
    }
    // Default to "us" if current layout not found
    return 0;
}

// Find current foreground color index in available_colors array
/**
 * @brief Find index of current foreground color in available options
 * 
 * Searches the available_colors array to find the index that matches
 * the current foreground color setting.
 * 
 * @return Index in available_colors array, or 0 if not found
 */
static unsigned int find_current_fg_color_index(void)
{
    GFX_COL current_fg = gfx_get_fg();
    for (unsigned int i = 0; i < num_colors; i++)
    {
        if (available_colors[i] == current_fg)
        {
            return i;
        }
    }
    // Default to GRAY if current color not found
    return 7; // GRAY index
}

// Find current background color index in available_colors array
/**
 * @brief Find index of current background color in available options
 * 
 * Searches the available_colors array to find the index that matches
 * the current background color setting.
 * 
 * @return Index in available_colors array, or 0 if not found
 */
static unsigned int find_current_bg_color_index(void)
{
    GFX_COL current_bg = gfx_get_bg();
    for (unsigned int i = 0; i < num_colors; i++)
    {
        if (available_colors[i] == current_bg)
        {
            return i;
        }
    }
    // Default to BLACK if current color not found
    return 0; // BLACK index
}

// Find current resolution index based on config
/**
 * @brief Find index of current display resolution in available options
 * 
 * Searches the available_resolutions array to find the index that matches
 * the current display width and height configuration.
 * 
 * @return Index in available_resolutions array, or 0 if not found
 */
static unsigned int find_current_resolution_index(void)
{
    for (unsigned int i = 0; i < num_resolutions; i++)
    {
        if ((resolution_widths[i] == PiGfxConfig.displayWidth) && 
            (resolution_heights[i] == PiGfxConfig.displayHeight))
        {
            return i;
        }
    }
    // Default to 1024x768 if not found
    return 1;
}

/**
 * @brief Enter setup/configuration mode
 * 
 * Activates the interactive setup mode where users can configure PiGFX settings
 * using keyboard navigation. This function:
 * - Saves current terminal state (cursor, colors, font, screen content)
 * - Initializes setup menu state with current configuration values
 * - Disables keyboard autorepeat to prevent navigation issues
 * - Switches to a suitable dialog font for the setup interface
 * - Saves screen buffer for restoration when exiting setup mode
 * 
 * The setup mode allows configuration of:
 * - UART baud rate
 * - Keyboard layout  
 * - Foreground/background colors
 * - Font selection
 * - Display resolution
 * - Cursor blinking
 * - Keyboard repeat settings
 * 
 * @note Only enters setup mode if not already active
 * @note Screen buffer saving may fail on memory constraints
 */
void setup_mode_enter(void)
       
{
    if (!setup_mode_active)
    {
        // Save cursor position and visibility state
        gfx_term_save_cursor();
        saved_cursor_visibility = gfx_term_get_cursor_visibility();
        
        // Save current colors
        saved_fg_color = gfx_get_fg();
        saved_bg_color = gfx_get_bg();
        
        // Save current font index from the font registry (before switching fonts)
        original_font_index = font_registry_get_current_index();
        saved_font_type = original_font_index;  // Use registry index for restoration
        
        // Disable keyboard autorepeat during setup mode to prevent key repeat issues
        keyboard_disable_autorepeat();
        
        // Initialize setup menu state (before switching dialog font)
        selected_item = 0;  // Start with Baudrate selected
        selected_baudrate_index = find_current_baudrate_index();
    selected_keyboard_index = find_current_keyboard_index();
    selected_switch_rxtx = PiGfxConfig.switchRxTx ? 1 : 0;
        selected_fg_color = find_current_fg_color_index();
        selected_bg_color = find_current_bg_color_index();
        selected_font_size = original_font_index;  // Use the saved original index
        selected_resolution_index = find_current_resolution_index();
        selected_cursor_blink = PiGfxConfig.cursorBlink ? 1 : 0;
        selected_auto_repeat = PiGfxConfig.keyboardAutorepeat ? 1 : 0; // Use config value
        selected_repeat_delay = PiGfxConfig.keyboardRepeatDelay;
        selected_repeat_rate = PiGfxConfig.keyboardRepeatRate;
    selected_send_crlf = PiGfxConfig.sendCRLF ? 1 : 0;
    selected_replace_lf_cr = PiGfxConfig.replaceLFwithCR ? 1 : 0;
    selected_sound_level = PiGfxConfig.soundLevel; // initialize sound level
    selected_key_click = PiGfxConfig.keyClick ? 1 : 0; // initialize key click
        
        // Reset the settings changed flag
        settings_changed = 0;
        
        // Hide cursor during setup mode
        gfx_term_set_cursor_visibility(0);
        
        // Allocate buffer for screen content and save BEFORE switching fonts
        unsigned int buffer_size = gfx_get_screen_buffer_size();
        saved_screen_buffer = nmalloc_malloc(buffer_size);
        
        if (saved_screen_buffer != 0)
        {
            // Save current screen content with original font
            gfx_save_screen_buffer(saved_screen_buffer);
        }
        
        // Switch to system default font (8x16 System Font at index 0) for dialog display
        switch_to_font_by_index(0);
        
        setup_mode_active = 1;
        needs_redraw = 1;
        setup_mode_draw();
    }
}

/**
 * @brief Exit setup mode and restore previous state
 * 
 * Deactivates setup mode and restores the terminal to its previous state
 * before setup mode was entered. This function:
 * - Restores original font, colors, and cursor settings
 * - Applies any configuration changes made during setup
 * - Restores screen content from saved buffer (if available)
 * - Re-enables keyboard autorepeat if it was enabled
 * - Clears any setup mode visual artifacts
 * 
 * If the user made changes during setup mode (settings_changed flag),
 * the new configuration is applied and saved. If no changes were made,
 * the original settings are preserved.
 * 
 * @note Only exits if setup mode is currently active
 * @note Screen restoration may fail if buffer was not saved
 * @note Configuration changes are applied immediately
 */
void setup_mode_exit(void)
{
    if (setup_mode_active)
    {
        setup_mode_active = 0;
        
        // First restore the original font before any screen operations
        switch_to_font_by_index(saved_font_type);
        
        // Restore original colors
        gfx_set_fg(saved_fg_color);
        gfx_set_bg(saved_bg_color);
        
        // Make sure cursor is hidden and clear any cursor artifacts
        gfx_term_set_cursor_visibility(0);
        
       
        // Try to restore screen content if buffer exists, otherwise just clear
        if (saved_screen_buffer != 0)
        {
            gfx_restore_screen_buffer(saved_screen_buffer);
            nmalloc_free(saved_screen_buffer);
            saved_screen_buffer = 0;
        }
        else
        {
            // Fallback: just clear screen if buffer allocation failed
            gfx_term_clear_screen();
        }
        
        // Restore cursor position and visibility
        gfx_term_restore_cursor();
        gfx_term_set_cursor_visibility(saved_cursor_visibility);
        
        // Re-enable keyboard autorepeat after setup mode
        keyboard_enable_autorepeat();
        
        // Force cursor rendering if cursor should be visible
        if (saved_cursor_visibility)
        {
            gfx_term_render_cursor();
        }
    }
}

/**
 * @brief Check if setup mode is currently active
 * 
 * Returns the current state of setup mode. Used by other parts of the system
 * to determine if they should handle input differently or avoid interfering
 * with setup mode operations.
 * 
 * @return 1 if setup mode is active, 0 if inactive
 */
unsigned char setup_mode_is_active(void)
{
    return setup_mode_active;
}

/**
 * @brief Handle keyboard input in setup mode
 * 
 * Processes keyboard input when setup mode is active, implementing the
 * navigation and configuration interface. Supported keys:
 * 
 * Navigation:
 * - UP/DOWN arrows: Navigate between configuration items
 * - LEFT/RIGHT arrows: Change values for selected item
 * 
 * Actions:
 * - ENTER: Apply changes and exit setup mode
 * - ESC: Cancel changes and exit setup mode
 * 
 * The function handles value cycling for each configuration type:
 * - Baud rates: Cycles through predefined values
 * - Keyboard layouts: Cycles through available layouts  
 * - Colors: Cycles through color palette
 * - Fonts: Cycles through font registry
 * - Resolution: Cycles through supported resolutions
 * - Boolean settings: Toggles between enabled/disabled
 * 
 * @param key Raw keyboard scancode/keycode
 * 
 * @note Only processes input if setup mode is active
 * @note Sets needs_redraw flag when display updates are required
 * @note Tracks configuration changes with settings_changed flag
 */
void setup_mode_handle_key(unsigned short key)
{
    if (!setup_mode_active) return;
    
    switch (key)
    {
        case KeyUp:
            // Move to previous item
            if (selected_item > 0)
            {
                selected_item--;
                needs_redraw = 1;
            }
            break;
            
        case KeyDown:
            // Move to next item
            if (selected_item < num_setup_items - 1)
            {
                selected_item++;
                needs_redraw = 1;
            }
            break;
            
        case KeyLeft:
            if (selected_item == 0) // Baudrate selected
            {
                if (selected_baudrate_index > 0)
                {
                    selected_baudrate_index--;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 1) // Switch Rx<>Tx
            {
                if (selected_switch_rxtx > 0)
                {
                    selected_switch_rxtx = 0;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 2) // Keyboard layout selected
            {
                if (selected_keyboard_index > 0)
                {
                    selected_keyboard_index--;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 3) // Foreground color selected
            {
                if (selected_fg_color > 0)
                {
                    selected_fg_color--;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 4) // Background color selected
            {
                if (selected_bg_color > 0)
                {
                    selected_bg_color--;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 5) // Font size selected
            {
                if (selected_font_size > 0)
                {
                    selected_font_size--;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 6) // Resolution selected
            {
                if (selected_resolution_index > 0)
                {
                    selected_resolution_index--;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 7) // Cursor Blink selected
            {
                if (selected_cursor_blink > 0)
                {
                    selected_cursor_blink = 0;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 8) // Auto Repeat selected
            {
                if (selected_auto_repeat > 0)
                {
                    selected_auto_repeat = 0;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 9) // Repeat Delay
            {
                if (selected_repeat_delay > 200)
                {
                    selected_repeat_delay -= 100;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 10) // Repeat Rate
            {
                if (selected_repeat_rate > 10)
                {
                    selected_repeat_rate -= 10;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 11) // Send CRLF toggle
            {
                if (selected_send_crlf > 0)
                {
                    selected_send_crlf = 0;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 12) // Replace LF with CR toggle
            {
                if (selected_replace_lf_cr > 0)
                {
                    selected_replace_lf_cr = 0;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 13) // Sound Level decrease
            {
                // Decrease in steps of 5, clamp at 0
                if (selected_sound_level > 0)
                {
                    selected_sound_level = (selected_sound_level >= 5) ? (selected_sound_level - 5) : 0;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 14) // Key Click toggle
            {
                if (selected_key_click > 0)
                {
                    selected_key_click = 0;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            break;
            
        case KeyRight:
            if (selected_item == 0) // Baudrate selected
            {
                if (selected_baudrate_index < num_baudrates - 1)
                {
                    selected_baudrate_index++;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 1) // Switch Rx<>Tx
            {
                if (selected_switch_rxtx < 1)
                {
                    selected_switch_rxtx = 1;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 2) // Keyboard layout selected
            {
                if (selected_keyboard_index < num_keyboards - 1)
                {
                    selected_keyboard_index++;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 3) // Foreground color selected
            {
                if (selected_fg_color < num_colors - 1)
                {
                    selected_fg_color++;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 4) // Background color selected
            {
                if (selected_bg_color < num_colors - 1)
                {
                    selected_bg_color++;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 5) // Font size selected
            {
                unsigned int font_count = font_registry_get_count();
                if (selected_font_size < font_count - 1)
                {
                    selected_font_size++;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 6) // Resolution selected
            {
                if (selected_resolution_index < num_resolutions - 1)
                {
                    selected_resolution_index++;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 7) // Cursor Blink selected
            {
                if (selected_cursor_blink < 1)
                {
                    selected_cursor_blink = 1;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 8) // Auto Repeat selected
            {
                if (selected_auto_repeat < 1)
                {
                    selected_auto_repeat = 1;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 9) // Repeat Delay
            {
                if (selected_repeat_delay < 1000)
                {
                    selected_repeat_delay += 100;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 10) // Repeat Rate
            {
                if (selected_repeat_rate < 50)
                {
                    selected_repeat_rate += 10;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 11) // Send CRLF toggle
            {
                if (selected_send_crlf < 1)
                {
                    selected_send_crlf = 1;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 12) // Replace LF with CR toggle
            {
                if (selected_replace_lf_cr < 1)
                {
                    selected_replace_lf_cr = 1;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 13) // Sound Level increase
            {
                // Increase in steps of 5, clamp at 100
                if (selected_sound_level < 100)
                {
                    unsigned int next = selected_sound_level + 5;
                    selected_sound_level = (next > 100) ? 100 : next;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 14) // Key Click toggle
            {
                if (selected_key_click < 1)
                {
                    selected_key_click = 1;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            break;
            
        case KeyEscape:
            // ESC: Exit without saving changes
            // HACK: Simulate an arrow key press to initialize dialog state before exit
            if (!settings_changed)
            {
                if (selected_item < num_setup_items - 1)
                {
                    selected_item++;
                    needs_redraw = 1;
                    // Force redraw to initialize dialog state
                    needs_redraw = 0;
                    setup_mode_draw();
                    selected_item--; // Restore original position
                }
            }
            
            // Exit without saving any changes - restore original saved state
            setup_mode_exit();
            break;
            
        case KeyReturn:
            // Enter: Save and apply settings if user made changes
            if (settings_changed)
            {
                // Check if font was changed for special handling
                unsigned char font_was_changed = (selected_font_size != original_font_index);
                
                // Check if resolution was changed
                unsigned int original_resolution_index = find_current_resolution_index();
                unsigned char resolution_was_changed = (selected_resolution_index != original_resolution_index);
                
                // Save the selected settings to config before exiting
                PiGfxConfig.uartBaudrate = available_baudrates[selected_baudrate_index];
                PiGfxConfig.keyboardLayout[0] = available_keyboards[selected_keyboard_index][0];
                PiGfxConfig.keyboardLayout[1] = available_keyboards[selected_keyboard_index][1];
                PiGfxConfig.keyboardLayout[2] = '\0'; // Null terminate
                PiGfxConfig.foregroundColor = available_colors[selected_fg_color];
                PiGfxConfig.backgroundColor = available_colors[selected_bg_color];
                PiGfxConfig.fontSelection = selected_font_size;
                PiGfxConfig.displayWidth = resolution_widths[selected_resolution_index];
                PiGfxConfig.displayHeight = resolution_heights[selected_resolution_index];
                PiGfxConfig.cursorBlink = selected_cursor_blink ? 1 : 0;
                PiGfxConfig.keyboardAutorepeat = selected_auto_repeat ? 1 : 0;
                PiGfxConfig.keyboardRepeatDelay = selected_repeat_delay;
                PiGfxConfig.keyboardRepeatRate = selected_repeat_rate;
                PiGfxConfig.sendCRLF = selected_send_crlf ? 1 : 0;
                PiGfxConfig.replaceLFwithCR = selected_replace_lf_cr ? 1 : 0;
                PiGfxConfig.switchRxTx = selected_switch_rxtx ? 1 : 0;
                PiGfxConfig.soundLevel = selected_sound_level;
                PiGfxConfig.keyClick = selected_key_click ? 1 : 0;
                
                // Update the saved colors so they don't get overwritten on exit
                saved_fg_color = available_colors[selected_fg_color];
                saved_bg_color = available_colors[selected_bg_color];
                
                // Update the saved font type to the selected font so exit doesn't restore the old one
                saved_font_type = selected_font_size;
                
                // Apply settings after setup mode has exited to avoid interference
                setup_mode_exit();

                // Apply cursor blinking setting immediately after setup
                gfx_term_set_cursor_blinking(PiGfxConfig.cursorBlink);

                // Apply keyboard repeat and autorepeat settings immediately
                if (PiGfxConfig.keyboardAutorepeat)
                    keyboard_enable_autorepeat();
                else
                    keyboard_disable_autorepeat();
                keyboard_set_repeat_delay(PiGfxConfig.keyboardRepeatDelay);
                keyboard_set_repeat_rate(PiGfxConfig.keyboardRepeatRate);
                fInitKeyboard(PiGfxConfig.keyboardLayout);

                // Re-initialize UART with new baudrate
                uart_init(PiGfxConfig.uartBaudrate);
                // Apply UART pin switch state immediately
                switch_uart_pins();
                
                // Handle resolution change if needed
                if (resolution_was_changed)
                {
                    gfx_term_putstring("Changing display resolution, please wait...\r\n");

                    // Re-initialize framebuffer with new resolution
                    initialize_framebuffer(PiGfxConfig.displayWidth, PiGfxConfig.displayHeight, 8);
                    gfx_term_clear_screen();
                    gfx_term_move_cursor(1, 1); // Move to row 1, column 1 (top-left)
                    
                    // Re-apply color and font settings after resolution change
                    gfx_set_default_fg(PiGfxConfig.foregroundColor);
                    gfx_set_default_bg(PiGfxConfig.backgroundColor);
                    gfx_set_fg(PiGfxConfig.foregroundColor);
                    gfx_set_bg(PiGfxConfig.backgroundColor);
                    
                    // Re-apply font selection
                    unsigned int font_count = font_registry_get_count();
                    if (PiGfxConfig.fontSelection < font_count)
                    {
                        gfx_term_set_font(PiGfxConfig.fontSelection);
                    }
                }
                // Only clear screen and reset cursor if font was changed (and resolution wasn't changed)
                else if (font_was_changed)
                {
                    gfx_term_clear_screen();
                    gfx_term_move_cursor(1, 1); // Move to row 1, column 1 (top-left)
                    
                    // Properly set up cursor at new position
                    gfx_term_save_cursor();  // Save content under cursor
                    if (saved_cursor_visibility) {
                        gfx_term_render_cursor();    // Show cursor if it should be visible
                    }
                }
                // For color changes only, cursor stays at current position with new colors
            }
            else
            {
                // HACK: If no changes were made, simulate an arrow key press to initialize
                // the dialog state properly before exiting
                if (selected_item < num_setup_items - 1)
                {
                    selected_item++;
                    needs_redraw = 1;
                    // Force redraw to initialize dialog state
                    needs_redraw = 0;
                    setup_mode_draw();
                    selected_item--; // Restore original position
                }
                
                // No changes made - exit with original saved state intact
                setup_mode_exit();
                // Apply cursor blinking setting immediately after setup
                gfx_term_set_cursor_blinking(PiGfxConfig.cursorBlink);
            }
            break;
            
        default:
            // Ignore other keys in setup mode
            break;
    }
    
    // Only redraw if something changed
    if (needs_redraw)
    {
        needs_redraw = 0;
        setup_mode_draw();
    }
}

/**
 * @brief Helper function to draw a menu item with proper highlighting
 * 
 * Draws a menu item label and value with appropriate colors based on selection state.
 * Selected items use reverse colors (background becomes foreground and vice versa).
 * 
 * @param row Terminal row position
 * @param content_col Column position for the label
 * @param value_col Column position for the value
 * @param label_width Width to clear for the label
 * @param value_width Width to clear for the value
 * @param label_text The label text to display
 * @param value_text The value text to display
 * @param is_selected Whether this item is currently selected
 * @param normal_fg Normal foreground color
 * @param normal_bg Normal background color
 */
static void draw_menu_item(unsigned int row, unsigned int content_col, unsigned int value_col,
                          unsigned int label_width, unsigned int value_width,
                          const char* label_text, const char* value_text,
                          unsigned char is_selected, GFX_COL normal_fg, GFX_COL normal_bg)
{
    if (is_selected)
    {
        // Selected item - reverse colors
        gfx_set_fg(normal_bg);
        gfx_set_bg(normal_fg);
        draw_text_at_with_bg(row, content_col, label_text, label_width);
        draw_text_at_with_bg(row, value_col, value_text, value_width);
    }
    else
    {
        // Not selected - normal colors
        gfx_set_fg(normal_fg);
        gfx_set_bg(normal_bg);
        draw_text_at(row, content_col, label_text);
        draw_text_at(row, value_col, value_text);
    }
}

/**
 * @brief Helper function to draw a menu item with integer value
 * 
 * Similar to draw_menu_item but for integer values that need formatting.
 * 
 * @param row Terminal row position
 * @param content_col Column position for the label
 * @param value_col Column position for the value
 * @param label_width Width to clear for the label
 * @param value_width Width to clear for the value
 * @param label_text The label text to display
 * @param value The integer value to display
 * @param suffix Optional suffix to append after the number (e.g., "ms", "Hz", "%")
 * @param is_selected Whether this item is currently selected
 * @param normal_fg Normal foreground color
 * @param normal_bg Normal background color
 */
static void draw_menu_item_int(unsigned int row, unsigned int content_col, unsigned int value_col,
                              unsigned int label_width, unsigned int value_width,
                              const char* label_text, unsigned int value, const char* suffix,
                              unsigned char is_selected, GFX_COL normal_fg, GFX_COL normal_bg)
{
    // Convert integer to string with optional suffix
    char value_buffer[16];
    char* p = value_buffer + sizeof(value_buffer) - 1;
    *p = '\0';
    
    // Add suffix first if provided
    if (suffix && *suffix)
    {
        const char* s = suffix;
        while (*s) s++; // Find end of suffix
        while (s > suffix) *(--p) = *(--s); // Copy suffix backwards
    }
    
    // Add number
    if (value == 0)
    {
        *(--p) = '0';
    }
    else
    {
        unsigned int temp = value;
        while (temp > 0)
        {
            *(--p) = '0' + (temp % 10);
            temp /= 10;
        }
    }
    
    draw_menu_item(row, content_col, value_col, label_width, value_width,
                   label_text, p, is_selected, normal_fg, normal_bg);
}

/**
 * @brief Helper function to draw color preview menu items
 * 
 * Special handling for color selection items that show color previews.
 * 
 * @param row Terminal row position
 * @param content_col Column position for the label
 * @param value_col Column position for the value
 * @param label_width Width to clear for the label
 * @param value_width Width to clear for the value
 * @param label_text The label text to display
 * @param color_index Index of the selected color
 * @param is_foreground Whether this is foreground (true) or background (false) color
 * @param is_selected Whether this item is currently selected
 * @param normal_fg Normal foreground color
 * @param normal_bg Normal background color
 */
static void draw_color_menu_item(unsigned int row, unsigned int content_col, unsigned int value_col,
                                unsigned int label_width, unsigned int value_width,
                                const char* label_text, unsigned int color_index, unsigned char is_foreground,
                                unsigned char is_selected, GFX_COL normal_fg, GFX_COL normal_bg)
{
    if (is_selected)
    {
        // Selected item - reverse colors, no preview
        gfx_set_fg(normal_bg);
        gfx_set_bg(normal_fg);
        draw_text_at_with_bg(row, content_col, label_text, label_width);
        draw_text_at_with_bg(row, value_col, color_names[color_index], value_width);
    }
    else
    {
        // Not selected - show color preview
        gfx_set_fg(normal_fg);
        gfx_set_bg(normal_bg);
        draw_text_at(row, content_col, label_text);
        
        if (is_foreground)
        {
            // Show color as foreground with normal background
            gfx_set_fg(available_colors[color_index]);
            gfx_set_bg(normal_bg);
        }
        else
        {
            // Show color as background with normal foreground
            gfx_set_fg(normal_fg);
            gfx_set_bg(available_colors[color_index]);
        }
        draw_text_at(row, value_col, color_names[color_index]);
    }
}

/**
 * @brief Render the setup mode user interface
 * 
 * Draws the complete setup mode interface on the screen using a clean two-color scheme
 * that adapts to the user's current terminal colors. The interface includes:
 * - Configuration menu with all available options
 * - Current values for each configuration parameter  
 * - Visual highlighting of the currently selected item (reverse colors)
 * - Instructions for navigation and control keys
 * - Real-time preview of color changes
 * 
 * The color scheme uses only two colors throughout:
 * - Normal items: current foreground on current background
 * - Selected items: current background on current foreground (reverse)
 * - Color previews: actual colors for foreground/background selection
 */
void setup_mode_draw(void)
{
    unsigned int screen_width, screen_height;
    unsigned int term_rows, term_cols;
    
    // Get screen dimensions
    gfx_get_gfx_size(&screen_width, &screen_height);
    gfx_get_term_size(&term_rows, &term_cols);
    
    // Save current colors and use consistent color scheme
    GFX_COL normal_fg = saved_fg_color;  // Use saved colors from when setup started
    GFX_COL normal_bg = saved_bg_color;
    
    // Fully clear the entire screen area in pixels to the background color
    gfx_set_fg(normal_bg);
    gfx_fill_rect(0, 0, screen_width, screen_height);
    
    // Use character-cell aligned layout for crisp rendering
    const unsigned int font_px_w = 8;
    const unsigned int font_px_h = 16;

    // Content sizing in character cells
    const unsigned int label_width = 23;       // columns for left labels (includes gap before values)
    const unsigned int value_width = 8;        // columns for right values
    const unsigned int content_width_cols = label_width + value_width;

    // Box sizing in character cells (inner padding of 2 cols on each side)
    const unsigned int min_box_cols = 48;      // ensure enough space for two instruction columns
    const unsigned int box_inner_cols = (content_width_cols + 4) < min_box_cols ? min_box_cols : (content_width_cols + 4);
    unsigned int box_char_cols = box_inner_cols;

    // Vertical sizing: top pad + title + spacer + items + spacer + instructions(2) + bottom pad
    const unsigned int top_pad_rows = 1;
    const unsigned int title_rows = 1;
    const unsigned int spacer_after_title = 1;
    const unsigned int items_rows = num_setup_items;
    const unsigned int spacer_before_instructions = 1;
    const unsigned int instruction_rows = 2;
    const unsigned int bottom_pad_rows = 1;
    const unsigned int box_char_rows = top_pad_rows + title_rows + spacer_after_title + items_rows + spacer_before_instructions + instruction_rows + bottom_pad_rows;

    // Center the box in character grid
    unsigned int box_char_x = (term_cols > box_char_cols) ? ((term_cols - box_char_cols) / 2) : 0;
    unsigned int box_char_y = (term_rows > box_char_rows) ? ((term_rows - box_char_rows) / 2) : 0;

    // Convert to pixels for drawing borders/background
    unsigned int box_x = box_char_x * font_px_w;
    unsigned int box_y = box_char_y * font_px_h;
    unsigned int box_width = box_char_cols * font_px_w;
    unsigned int box_height = box_char_rows * font_px_h;
    
    // Draw setup box border using foreground color
    gfx_set_fg(normal_fg);
    gfx_fill_rect(box_x, box_y, box_width, 2);                    // top border
    gfx_fill_rect(box_x, box_y + box_height - 2, box_width, 2);   // bottom border
    gfx_fill_rect(box_x, box_y, 2, box_height);                   // left border
    gfx_fill_rect(box_x + box_width - 2, box_y, 2, box_height);   // right border
    
    // Fill box background with background color
    gfx_set_fg(normal_bg);
    gfx_fill_rect(box_x + 2, box_y + 2, box_width - 4, box_height - 4);
    
    // Calculate text positions in character cells
    unsigned int title_row = box_char_y + top_pad_rows;
    unsigned int inner_left_col = box_char_x + 2;
    unsigned int inner_right_col = box_char_x + box_char_cols - 3;
    unsigned int content_row = title_row + title_rows + spacer_after_title;

    // Center the content area horizontally within inner box area
    unsigned int inner_width_cols = (box_char_cols - 4);
    unsigned int content_col = inner_left_col + (inner_width_cols > content_width_cols ? (inner_width_cols - content_width_cols) / 2 : 0);
    unsigned int value_col = content_col + label_width;
    
    // Draw title - centered
    gfx_set_fg(normal_fg);
    gfx_set_bg(normal_bg);
    const unsigned int title_len = 13; // "Pi VT100 Setup"
    unsigned int title_center_col = box_char_x + (box_char_cols - title_len) / 2;
    draw_text_at(title_row, title_center_col, "Pi VT100 Setup");
    
    // Draw all menu items using helper functions
    unsigned int current_row = content_row;
    
    // 0: Baud Rate
    char baudrate_str[16];
    char* p = baudrate_str + sizeof(baudrate_str) - 1;
    *p = '\0';
    unsigned int baud = available_baudrates[selected_baudrate_index];
    if (baud == 0) { *(--p) = '0'; }
    else { while (baud > 0) { *(--p) = '0' + (baud % 10); baud /= 10; } }
    draw_menu_item(current_row++, content_col, value_col, label_width, value_width,
                   "Baud Rate", p, (selected_item == 0), normal_fg, normal_bg);
    
    // 1: Switch Rx<>Tx
    draw_menu_item(current_row++, content_col, value_col, label_width, value_width,
                   "Switch Rx<>Tx", selected_switch_rxtx ? "On" : "Off", 
                   (selected_item == 1), normal_fg, normal_bg);
    
    // 2: Keyboard Layout
    draw_menu_item(current_row++, content_col, value_col, label_width, value_width,
                   "Keyboard Layout", available_keyboards[selected_keyboard_index], 
                   (selected_item == 2), normal_fg, normal_bg);
    
    // 3: Foreground Color (with preview)
    draw_color_menu_item(current_row++, content_col, value_col, label_width, value_width,
                        "Foreground", selected_fg_color, 1, 
                        (selected_item == 3), normal_fg, normal_bg);
    
    // 4: Background Color (with preview)
    draw_color_menu_item(current_row++, content_col, value_col, label_width, value_width,
                        "Background", selected_bg_color, 0, 
                        (selected_item == 4), normal_fg, normal_bg);
    
    // 5: Font Size
    const font_descriptor_t* font_info = font_registry_get_info(selected_font_size);
    const char* font_name = (font_info != NULL && strlen(font_info->name) != 0) ? font_info->name : "Unknown";
    draw_menu_item(current_row++, content_col, value_col, label_width, 12,
                   "Font Size", font_name, (selected_item == 5), normal_fg, normal_bg);
    
    // 6: Resolution
    draw_menu_item(current_row++, content_col, value_col, label_width, 10,
                   "Resolution", available_resolutions[selected_resolution_index], 
                   (selected_item == 6), normal_fg, normal_bg);
    
    // 7: Cursor Blink
    draw_menu_item(current_row++, content_col, value_col, label_width, value_width,
                   "Cursor Blink", selected_cursor_blink ? "On" : "Off", 
                   (selected_item == 7), normal_fg, normal_bg);
    
    // 8: Auto Repeat
    draw_menu_item(current_row++, content_col, value_col, label_width, value_width,
                   "Auto Repeat", selected_auto_repeat ? "On" : "Off", 
                   (selected_item == 8), normal_fg, normal_bg);
    
    // 9: Repeat Delay
    draw_menu_item_int(current_row++, content_col, value_col, label_width, value_width,
                      "Repeat Delay", selected_repeat_delay, "ms", 
                      (selected_item == 9), normal_fg, normal_bg);
    
    // 10: Repeat Rate
    draw_menu_item_int(current_row++, content_col, value_col, label_width, value_width,
                      "Repeat Rate", selected_repeat_rate, "Hz", 
                      (selected_item == 10), normal_fg, normal_bg);
    
    // 11: Send CRLF
    draw_menu_item(current_row++, content_col, value_col, label_width, value_width,
                   "Send CRLF", selected_send_crlf ? "On" : "Off", 
                   (selected_item == 11), normal_fg, normal_bg);
    
    // 12: Replace LF with CR
    draw_menu_item(current_row++, content_col, value_col, label_width, value_width,
                   "Replace LF with CR", selected_replace_lf_cr ? "On" : "Off", 
                   (selected_item == 12), normal_fg, normal_bg);
    
    // 13: Sound Level
    draw_menu_item_int(current_row++, content_col, value_col, label_width, 5,
                      "Sound Level", selected_sound_level, "%", 
                      (selected_item == 13), normal_fg, normal_bg);
    
    // 14: Key Click
    draw_menu_item(current_row++, content_col, value_col, label_width, value_width,
                   "Key Click", selected_key_click ? "On" : "Off", 
                   (selected_item == 14), normal_fg, normal_bg);

    // Draw instructions
    unsigned int instruction_row = content_row + items_rows + spacer_before_instructions;
    gfx_set_fg(normal_fg);
    gfx_set_bg(normal_bg);

    // Left column instructions
    unsigned int left_instruction_col = inner_left_col + 2;
    draw_text_at(instruction_row, left_instruction_col, "Up/Down: Select");
    draw_text_at(instruction_row + 1, left_instruction_col, "ESC: Exit");

    // Right column instructions
    const unsigned int right_text_len_top = 19;    // "Left/Right: Change"
    const unsigned int right_text_len_bottom = 18; // "Enter: Save & Exit"
    unsigned int right_instruction_col_top = (inner_right_col + 1 > right_text_len_top) ? 
        (inner_right_col + 1 - right_text_len_top) : inner_left_col + inner_width_cols / 2;
    unsigned int right_instruction_col_bottom = (inner_right_col + 1 > right_text_len_bottom + 1) ? 
        (inner_right_col - right_text_len_bottom) : (right_instruction_col_top > 0 ? right_instruction_col_top - 1 : 0);
    
    draw_text_at(instruction_row, right_instruction_col_top, "Left/Right: Change");
    draw_text_at(instruction_row + 1, right_instruction_col_bottom, "Enter: Save & Exit");
}
