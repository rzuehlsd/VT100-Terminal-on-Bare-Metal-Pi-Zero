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
static unsigned int selected_keyboard_index = 0;
static unsigned int selected_fg_color = 0;
static unsigned int selected_bg_color = 0;
static unsigned int selected_font_size = 0;
static unsigned int selected_resolution_index = 0;
static unsigned int selected_cursor_blink = 1;
static unsigned int selected_auto_repeat = 1;  // Default auto repeat
static unsigned int selected_repeat_delay = 500;
static unsigned int selected_repeat_rate = 10;
static const unsigned int num_setup_items = 10;  // Number of setup items (added repeat delay/rate)

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

// Available colors for foreground/background
static const GFX_COL available_colors[] = {
    BLACK, DARKRED, DARKGREEN, DARKYELLOW, DARKBLUE, DARKMAGENTA, DARKCYAN, GRAY,
    DARKGRAY, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE
};
static const char* color_names[] = {
    "Black", "DkRed", "DkGrn", "DkYel", "DkBlu", "DkMag", "DkCyn", "Gray",
    "DkGry", "Red", "Green", "Yellow", "Blue", "Magent", "Cyan", "White"
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
static void switch_to_font_by_index(int font_index)
{
    gfx_term_set_font(font_index);
}

// Helper function to draw text at specific position without affecting cursor
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
static void draw_int_at(unsigned int row, unsigned int col, unsigned int value)
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
static void draw_int_at_with_bg(unsigned int row, unsigned int col, unsigned int value, unsigned int length)
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
        selected_fg_color = find_current_fg_color_index();
        selected_bg_color = find_current_bg_color_index();
        selected_font_size = original_font_index;  // Use the saved original index
        selected_resolution_index = find_current_resolution_index();
        selected_cursor_blink = PiGfxConfig.cursorBlink ? 1 : 0;
        selected_auto_repeat = PiGfxConfig.keyboardAutorepeat ? 1 : 0; // Use config value
        selected_repeat_delay = PiGfxConfig.keyboardRepeatDelay;
        selected_repeat_rate = PiGfxConfig.keyboardRepeatRate;
        
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

unsigned char setup_mode_is_active(void)
{
    return setup_mode_active;
}

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
            else if (selected_item == 1) // Keyboard layout selected
            {
                if (selected_keyboard_index > 0)
                {
                    selected_keyboard_index--;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 2) // Foreground color selected
            {
                if (selected_fg_color > 0)
                {
                    selected_fg_color--;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 3) // Background color selected
            {
                if (selected_bg_color > 0)
                {
                    selected_bg_color--;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 4) // Font size selected
            {
                if (selected_font_size > 0)
                {
                    selected_font_size--;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 5) // Resolution selected
            {
                if (selected_resolution_index > 0)
                {
                    selected_resolution_index--;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 6) // Cursor Blink selected
            {
                if (selected_cursor_blink > 0)
                {
                    selected_cursor_blink = 0;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 7) // Cursor Blink selected
            {
                if (selected_auto_repeat > 0)
                {
                    selected_auto_repeat = 0;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 8) // Repeat Delay
            {
                if (selected_repeat_delay > 200)
                {
                    selected_repeat_delay -= 100;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 9) // Repeat Rate
            {
                if (selected_repeat_rate > 10)
                {
                    selected_repeat_rate -= 10;
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
            else if (selected_item == 1) // Keyboard layout selected
            {
                if (selected_keyboard_index < num_keyboards - 1)
                {
                    selected_keyboard_index++;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 2) // Foreground color selected
            {
                if (selected_fg_color < num_colors - 1)
                {
                    selected_fg_color++;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 3) // Background color selected
            {
                if (selected_bg_color < num_colors - 1)
                {
                    selected_bg_color++;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 4) // Font size selected
            {
                unsigned int font_count = font_registry_get_count();
                if (selected_font_size < font_count - 1)
                {
                    selected_font_size++;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 5) // Resolution selected
            {
                if (selected_resolution_index < num_resolutions - 1)
                {
                    selected_resolution_index++;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 6) // Cursor Blink selected
            {
                if (selected_cursor_blink < 1)
                {
                    selected_cursor_blink = 1;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 7) // Cursor Blink selected
            {
                if (selected_auto_repeat < 1)
                {
                    selected_auto_repeat = 1;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 8) // Repeat Delay
            {
                if (selected_repeat_delay < 1000)
                {
                    selected_repeat_delay += 100;
                    settings_changed = 1;
                    needs_redraw = 1;
                }
            }
            else if (selected_item == 9) // Repeat Rate
            {
                if (selected_repeat_rate < 50)
                {
                    selected_repeat_rate += 10;
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

void setup_mode_draw(void)
{
    unsigned int screen_width, screen_height;
    unsigned int term_rows, term_cols;
    
    // Get screen dimensions
    gfx_get_gfx_size(&screen_width, &screen_height);
    gfx_get_term_size(&term_rows, &term_cols);
    
    // Save current background color and restore original background for screen clear
    GFX_COL current_bg = gfx_get_bg();
    gfx_set_bg(saved_bg_color);
    
    // Clear screen with original background color
    gfx_term_clear_screen();
    
    // Restore working background color
    gfx_set_bg(current_bg);
    
    // Calculate setup box dimensions (centered)
    unsigned int box_width = 450;  // pixels - increased to accommodate longer font names
    unsigned int box_height = 300; // pixels - increased for additional instruction line
    unsigned int box_x = (screen_width - box_width) / 2;
    unsigned int box_y = (screen_height - box_height) / 2;
    
    // Draw setup box border
    gfx_set_fg(WHITE);
    gfx_fill_rect(box_x, box_y, box_width, 2);                    // top border
    gfx_fill_rect(box_x, box_y + box_height - 2, box_width, 2);   // bottom border
    gfx_fill_rect(box_x, box_y, 2, box_height);                   // left border
    gfx_fill_rect(box_x + box_width - 2, box_y, 2, box_height);   // right border
    
    // Fill box background
    gfx_set_fg(BLUE);
    gfx_fill_rect(box_x + 2, box_y + 2, box_width - 4, box_height - 4);
    
    // Calculate text positions (row/col based) for 2-column layout
    unsigned int title_row = box_y / 16 + 1;  // assuming 16px font height
    unsigned int title_col = box_x / 8 + 2;   // assuming 8px font width
    unsigned int content_row = title_row + 2;
    unsigned int content_col = title_col + 2;
    unsigned int label_width = 16;  // Fixed width for label column
    unsigned int value_col = content_col + label_width;  // Start of value column
    
    // Draw title using direct character drawing - centered
    gfx_set_fg(YELLOW);
    gfx_set_bg(BLUE);
    // Center "Pi VT100 Setup" in the dialog box (13 characters)
    unsigned int title_center_col = title_col + (box_width / 8 - 13) / 2;
    draw_text_at(title_row, title_center_col, "Pi VT100 Setup");
    
    // Draw baud rate label and value with selection highlighting
    if (selected_item == 0)
    {
        // Selected item - black on white background
        gfx_set_fg(BLACK);
        gfx_set_bg(WHITE);
        draw_text_at_with_bg(content_row, content_col, "Baud Rate", label_width);
        
        // Value also black on white background
        gfx_set_fg(BLACK);
        gfx_set_bg(WHITE);
        draw_int_at_with_bg(content_row, value_col, available_baudrates[selected_baudrate_index], 8);
    }
    else
    {
        // Not selected - normal colors
        gfx_set_fg(WHITE);
        gfx_set_bg(BLUE);
        draw_text_at(content_row, content_col, "Baud Rate");
        
        gfx_set_fg(GREEN);
        gfx_set_bg(BLUE);
        draw_int_at(content_row, value_col, available_baudrates[selected_baudrate_index]);
    }
    
    // Draw keyboard layout label and value with selection highlighting
    if (selected_item == 1)
    {
        // Selected item - black on white background
        gfx_set_fg(BLACK);
        gfx_set_bg(WHITE);
        draw_text_at_with_bg(content_row + 1, content_col, "Keyboard Layout", label_width);
        
        gfx_set_fg(BLACK);
        gfx_set_bg(WHITE);
        draw_text_at_with_bg(content_row + 1, value_col, available_keyboards[selected_keyboard_index], 8);
    }
    else
    {
        // Not selected - normal colors
        gfx_set_fg(WHITE);
        gfx_set_bg(BLUE);
        draw_text_at(content_row + 1, content_col, "Keyboard Layout");
        
        gfx_set_fg(GREEN);
        gfx_set_bg(BLUE);
        draw_text_at(content_row + 1, value_col, available_keyboards[selected_keyboard_index]);
    }
    
    // Draw foreground color label and value with selection highlighting
    if (selected_item == 2)
    {
        // Selected item - black on white background
        gfx_set_fg(BLACK);
        gfx_set_bg(WHITE);
        draw_text_at_with_bg(content_row + 2, content_col, "Foreground", label_width);
        
        gfx_set_fg(BLACK);
        gfx_set_bg(WHITE);
        draw_text_at_with_bg(content_row + 2, value_col, color_names[selected_fg_color], 8);
    }
    else
    {
        // Not selected - normal colors
        gfx_set_fg(WHITE);
        gfx_set_bg(BLUE);
        draw_text_at(content_row + 2, content_col, "Foreground");
        
        gfx_set_fg(available_colors[selected_fg_color]); // Show color in its own color
        gfx_set_bg(BLUE);
        draw_text_at(content_row + 2, value_col, color_names[selected_fg_color]);
    }
    
    // Draw background color label and value with selection highlighting
    if (selected_item == 3)
    {
        // Selected item - black on white background
        gfx_set_fg(BLACK);
        gfx_set_bg(WHITE);
        draw_text_at_with_bg(content_row + 3, content_col, "Background", label_width);
        
        gfx_set_fg(BLACK);
        gfx_set_bg(WHITE);
        draw_text_at_with_bg(content_row + 3, value_col, color_names[selected_bg_color], 8);
    }
    else
    {
        // Not selected - normal colors
        gfx_set_fg(WHITE);
        gfx_set_bg(BLUE);
        draw_text_at(content_row + 3, content_col, "Background");
        
        gfx_set_fg(WHITE);
        gfx_set_bg(available_colors[selected_bg_color]); // Show color as background
        draw_text_at(content_row + 3, value_col, color_names[selected_bg_color]);
    }
    
    // Draw font size label and value with selection highlighting
    if (selected_item == 4)
    {
        // Selected item - black on white background
        gfx_set_fg(BLACK);
        gfx_set_bg(WHITE);
        draw_text_at_with_bg(content_row + 4, content_col, "Font Size", label_width);
        
        gfx_set_fg(BLACK);
        gfx_set_bg(WHITE);
        
        // Get font info from registry
        const font_descriptor_t* font_info = font_registry_get_info(selected_font_size);
        if (font_info != NULL && strlen(font_info->name) != 0)
        {
            draw_text_at_with_bg(content_row + 4, value_col, font_info->name, 12);
        }
        else
        {
            draw_text_at_with_bg(content_row + 4, value_col, "Unknown", 12);
        }
    }
    else
    {
        // Not selected - normal colors
        gfx_set_fg(WHITE);
        gfx_set_bg(BLUE);
        draw_text_at(content_row + 4, content_col, "Font Size");
        
        gfx_set_fg(GREEN);
        gfx_set_bg(BLUE);
        
        // Get font info from registry
        const font_descriptor_t* font_info = font_registry_get_info(selected_font_size);
        if (font_info != NULL && strlen(font_info->name) != 0)
        {
            draw_text_at(content_row + 4, value_col, font_info->name);
        }
        else
        {
            draw_text_at(content_row + 4, value_col, "Unknown");
        }
    }
    
    // Draw resolution label and value with selection highlighting
    if (selected_item == 5)
    {
        // Selected item - black on white background
        gfx_set_fg(BLACK);
        gfx_set_bg(WHITE);
        draw_text_at_with_bg(content_row + 5, content_col, "Resolution", label_width);
        
        gfx_set_fg(BLACK);
        gfx_set_bg(WHITE);
        draw_text_at_with_bg(content_row + 5, value_col, available_resolutions[selected_resolution_index], 10);
    }
    else
    {
        // Not selected - normal colors
        gfx_set_fg(WHITE);
        gfx_set_bg(BLUE);
        draw_text_at(content_row + 5, content_col, "Resolution");
        
        gfx_set_fg(GREEN);
        gfx_set_bg(BLUE);
        draw_text_at(content_row + 5, value_col, available_resolutions[selected_resolution_index]);
    }
    
    // Draw cursor blink label and value with selection highlighting
    if (selected_item == 6)
    {
        // Selected item - black on white background
        gfx_set_fg(BLACK);
        gfx_set_bg(WHITE);
        draw_text_at_with_bg(content_row + 6, content_col, "Cursor Blink", label_width);
        gfx_set_fg(BLACK);
        gfx_set_bg(WHITE);
        draw_text_at_with_bg(content_row + 6, value_col, selected_cursor_blink ? "On " : "Off", 8);
    }
    else
    {
        // Not selected - normal colors
        gfx_set_fg(WHITE);
        gfx_set_bg(BLUE);
        draw_text_at(content_row + 6, content_col, "Cursor Blink");
        gfx_set_fg(GREEN);
        gfx_set_bg(BLUE);
        draw_text_at(content_row + 6, value_col, selected_cursor_blink ? "On" : "Off");
    }

    // Draw auto repeat label and value with selection highlighting
    if (selected_item == 7)
    {
        gfx_set_fg(BLACK);
        gfx_set_bg(WHITE);
        draw_text_at_with_bg(content_row + 7, content_col, "Auto Repeat", label_width);
        gfx_set_fg(BLACK);
        gfx_set_bg(WHITE);
        draw_text_at_with_bg(content_row + 7, value_col, selected_auto_repeat? "On" : "Off", 8);
    }
    else
    {
        gfx_set_fg(WHITE);
        gfx_set_bg(BLUE);
        draw_text_at(content_row + 7, content_col, "Auto Repeat");
        gfx_set_fg(GREEN);
        gfx_set_bg(BLUE);
        draw_text_at(content_row + 7, value_col, selected_auto_repeat ? "On" : "Off");
    }

    // Draw repeat delay label and value with selection highlighting
    if (selected_item == 8)
    {
        gfx_set_fg(BLACK);
        gfx_set_bg(WHITE);
        draw_text_at_with_bg(content_row + 8, content_col, "Repeat Delay", label_width);
        gfx_set_fg(BLACK);
        gfx_set_bg(WHITE);
        draw_int_at_with_bg(content_row + 8, value_col, selected_repeat_delay, 8);
        draw_text_at_with_bg(content_row + 8, value_col + 6, "ms", 3);
    }
    else
    {
        gfx_set_fg(WHITE);
        gfx_set_bg(BLUE);
        draw_text_at(content_row + 8, content_col, "Repeat Delay");
        gfx_set_fg(GREEN);
        gfx_set_bg(BLUE);
        draw_int_at(content_row + 8, value_col, selected_repeat_delay);
        draw_text_at(content_row + 8, value_col + 6, "ms");
    }

    // Draw repeat rate label and value with selection highlighting
    if (selected_item == 9)
    {
        gfx_set_fg(BLACK);
        gfx_set_bg(WHITE);
        draw_text_at_with_bg(content_row + 9, content_col, "Repeat Rate", label_width);
        gfx_set_fg(BLACK);
        gfx_set_bg(WHITE);
        draw_int_at_with_bg(content_row + 9, value_col, selected_repeat_rate, 8);
        draw_text_at_with_bg(content_row + 9, value_col + 6, "Hz", 3);
    }
    else
    {
        gfx_set_fg(WHITE);
        gfx_set_bg(BLUE);
        draw_text_at(content_row + 9, content_col, "Repeat Rate");
        gfx_set_fg(GREEN);
        gfx_set_bg(BLUE);
        draw_int_at(content_row + 9, value_col, selected_repeat_rate);
        draw_text_at(content_row + 9, value_col + 6, "Hz");
    }

    // Draw instructions in 2-column layout at bottom, 1 line above border
    unsigned int instruction_row = (box_y + box_height - 32) / 16;  // 2 lines from bottom (16px per line)
    gfx_set_fg(CYAN);
    gfx_set_bg(BLUE);
    
    // Left column instructions
    draw_text_at(instruction_row, content_col, "Up/Down: Select");
    draw_text_at(instruction_row + 1, content_col, "ESC: Exit");
    
    // Right column instructions  
    unsigned int right_instruction_col = content_col + 20;
    draw_text_at(instruction_row, right_instruction_col, "Left/Right: Change");
    draw_text_at(instruction_row + 1, right_instruction_col, "Enter: Save & Exit");
}
