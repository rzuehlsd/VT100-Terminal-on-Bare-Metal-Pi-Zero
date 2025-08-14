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

static unsigned char setup_mode_active = 0;
static void* saved_screen_buffer = 0;
static unsigned char saved_cursor_visibility = 0;
static GFX_COL saved_fg_color = 0;
static GFX_COL saved_bg_color = 0;

// Setup menu state
static unsigned int selected_item = 0;  // 0 = Baudrate, 1 = Keyboard, 2 = Foreground, 3 = Background
static unsigned int selected_baudrate_index = 0;
static unsigned int selected_keyboard_index = 0;
static unsigned int selected_fg_color = 0;
static unsigned int selected_bg_color = 0;
static const unsigned int num_setup_items = 4;  // Number of setup items

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
        
        // Initialize setup menu state
        selected_item = 0;  // Start with Baudrate selected
        selected_baudrate_index = find_current_baudrate_index();
        selected_keyboard_index = find_current_keyboard_index();
        selected_fg_color = find_current_fg_color_index();
        selected_bg_color = find_current_bg_color_index();
        
        // Hide cursor during setup mode
        gfx_term_set_cursor_visibility(0);
        
        // Allocate buffer for screen content
        unsigned int buffer_size = gfx_get_screen_buffer_size();
        saved_screen_buffer = nmalloc_malloc(buffer_size);
        
        if (saved_screen_buffer != 0)
        {
            // Save current screen content
            gfx_save_screen_buffer(saved_screen_buffer);
        }
        
        setup_mode_active = 1;
        setup_mode_draw();
    }
}

void setup_mode_exit(void)
{
    if (setup_mode_active)
    {
        setup_mode_active = 0;
        
        // Make sure cursor is hidden and clear any cursor artifacts
        gfx_term_set_cursor_visibility(0);
        
        // Restore screen content if buffer exists
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
        
        // Restore original colors
        gfx_set_fg(saved_fg_color);
        gfx_set_bg(saved_bg_color);
        
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
                setup_mode_draw();  // Redraw to show new selection
            }
            break;
            
        case KeyDown:
            // Move to next item
            if (selected_item < num_setup_items - 1)
            {
                selected_item++;
                setup_mode_draw();  // Redraw to show new selection
            }
            break;
            
        case KeyLeft:
            if (selected_item == 0) // Baudrate selected
            {
                if (selected_baudrate_index > 0)
                {
                    selected_baudrate_index--;
                    setup_mode_draw();  // Redraw to show new selection
                }
            }
            else if (selected_item == 1) // Keyboard layout selected
            {
                if (selected_keyboard_index > 0)
                {
                    selected_keyboard_index--;
                    setup_mode_draw();  // Redraw to show new selection
                }
            }
            else if (selected_item == 2) // Foreground color selected
            {
                if (selected_fg_color > 0)
                {
                    selected_fg_color--;
                    setup_mode_draw();  // Redraw to show new selection
                }
            }
            else if (selected_item == 3) // Background color selected
            {
                if (selected_bg_color > 0)
                {
                    selected_bg_color--;
                    setup_mode_draw();  // Redraw to show new selection
                }
            }
            break;
            
        case KeyRight:
            if (selected_item == 0) // Baudrate selected
            {
                if (selected_baudrate_index < num_baudrates - 1)
                {
                    selected_baudrate_index++;
                    setup_mode_draw();  // Redraw to show new selection
                }
            }
            else if (selected_item == 1) // Keyboard layout selected
            {
                if (selected_keyboard_index < num_keyboards - 1)
                {
                    selected_keyboard_index++;
                    setup_mode_draw();  // Redraw to show new selection
                }
            }
            else if (selected_item == 2) // Foreground color selected
            {
                if (selected_fg_color < num_colors - 1)
                {
                    selected_fg_color++;
                    setup_mode_draw();  // Redraw to show new selection
                }
            }
            else if (selected_item == 3) // Background color selected
            {
                if (selected_bg_color < num_colors - 1)
                {
                    selected_bg_color++;
                    setup_mode_draw();  // Redraw to show new selection
                }
            }
            break;
            
        case KeyEscape:
            // Save the selected settings to config before exiting
            PiGfxConfig.uartBaudrate = available_baudrates[selected_baudrate_index];
            PiGfxConfig.keyboardLayout[0] = available_keyboards[selected_keyboard_index][0];
            PiGfxConfig.keyboardLayout[1] = available_keyboards[selected_keyboard_index][1];
            PiGfxConfig.keyboardLayout[2] = '\0'; // Null terminate
            
            // Apply the new keyboard layout immediately
            fInitKeyboard(PiGfxConfig.keyboardLayout);
            
            // Apply the new baudrate to UART immediately
            uart_init(PiGfxConfig.uartBaudrate);
            
            // Update the saved colors so they don't get overwritten on exit
            saved_fg_color = available_colors[selected_fg_color];
            saved_bg_color = available_colors[selected_bg_color];
            
            setup_mode_exit();
            break;
            
        default:
            // Ignore other keys in setup mode
            break;
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
    unsigned int box_width = 400;  // pixels
    unsigned int box_height = 260; // pixels - increased for color selection items
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
    
    // Calculate text positions (row/col based)
    unsigned int title_row = box_y / 16 + 1;  // assuming 16px font height
    unsigned int title_col = box_x / 8 + 2;   // assuming 8px font width
    unsigned int content_row = title_row + 2;
    unsigned int content_col = title_col + 2;
    
    // Draw title using direct character drawing - centered
    gfx_set_fg(YELLOW);
    gfx_set_bg(BLUE);
    // Center "PiVT 100 Setup" in the dialog box (13 characters)
    unsigned int title_center_col = title_col + (box_width / 8 - 13) / 2;
    draw_text_at(title_row, title_center_col, "Pi VT100 Setup");
    
    // Draw baudrate label and value with selection highlighting
    if (selected_item == 0)
    {
        // Selected item - black on white background
        gfx_set_fg(BLACK);
        gfx_set_bg(WHITE);
        draw_text_at_with_bg(content_row, content_col, "Baudrate: ", 10);
        
        // Value also black on white background
        gfx_set_fg(BLACK);
        gfx_set_bg(WHITE);
        draw_int_at_with_bg(content_row, content_col + 10, available_baudrates[selected_baudrate_index], 8);
    }
    else
    {
        // Not selected - normal colors
        gfx_set_fg(WHITE);
        gfx_set_bg(BLUE);
        draw_text_at(content_row, content_col, "Baudrate: ");
        
        gfx_set_fg(GREEN);
        gfx_set_bg(BLUE);
        draw_int_at(content_row, content_col + 10, available_baudrates[selected_baudrate_index]);
    }
    
    // Draw keyboard layout label and value with selection highlighting
    if (selected_item == 1)
    {
        // Selected item - black on white background
        gfx_set_fg(BLACK);
        gfx_set_bg(WHITE);
        draw_text_at_with_bg(content_row + 1, content_col, "Keyboard: ", 10);
        
        gfx_set_fg(BLACK);
        gfx_set_bg(WHITE);
        draw_text_at_with_bg(content_row + 1, content_col + 10, available_keyboards[selected_keyboard_index], 8);
    }
    else
    {
        // Not selected - normal colors
        gfx_set_fg(WHITE);
        gfx_set_bg(BLUE);
        draw_text_at(content_row + 1, content_col, "Keyboard: ");
        
        gfx_set_fg(GREEN);
        gfx_set_bg(BLUE);
        draw_text_at(content_row + 1, content_col + 10, available_keyboards[selected_keyboard_index]);
    }
    
    // Draw foreground color label and value with selection highlighting
    if (selected_item == 2)
    {
        // Selected item - black on white background
        gfx_set_fg(BLACK);
        gfx_set_bg(WHITE);
        draw_text_at_with_bg(content_row + 2, content_col, "FG Color: ", 10);
        
        gfx_set_fg(BLACK);
        gfx_set_bg(WHITE);
        draw_text_at_with_bg(content_row + 2, content_col + 10, color_names[selected_fg_color], 8);
    }
    else
    {
        // Not selected - normal colors
        gfx_set_fg(WHITE);
        gfx_set_bg(BLUE);
        draw_text_at(content_row + 2, content_col, "FG Color: ");
        
        gfx_set_fg(available_colors[selected_fg_color]); // Show color in its own color
        gfx_set_bg(BLUE);
        draw_text_at(content_row + 2, content_col + 10, color_names[selected_fg_color]);
    }
    
    // Draw background color label and value with selection highlighting
    if (selected_item == 3)
    {
        // Selected item - black on white background
        gfx_set_fg(BLACK);
        gfx_set_bg(WHITE);
        draw_text_at_with_bg(content_row + 3, content_col, "BG Color: ", 10);
        
        gfx_set_fg(BLACK);
        gfx_set_bg(WHITE);
        draw_text_at_with_bg(content_row + 3, content_col + 10, color_names[selected_bg_color], 8);
    }
    else
    {
        // Not selected - normal colors
        gfx_set_fg(WHITE);
        gfx_set_bg(BLUE);
        draw_text_at(content_row + 3, content_col, "BG Color: ");
        
        gfx_set_fg(WHITE);
        gfx_set_bg(available_colors[selected_bg_color]); // Show color as background
        draw_text_at(content_row + 3, content_col + 10, color_names[selected_bg_color]);
    }
    
    // Draw instructions using direct character drawing
    gfx_set_fg(CYAN);
    gfx_set_bg(BLUE);
    draw_text_at(content_row + 5, content_col, "Up/Down: select item");
    draw_text_at(content_row + 6, content_col, "Left/Right: change value");
    draw_text_at(content_row + 7, content_col, "ESC: save and exit");
}
