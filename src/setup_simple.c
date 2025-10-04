//
// setup_simple.c
// Simplified setup mode functions for PiGFX configuration
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
#include "keyboard.h"
#include "uart.h"
#include "font_registry.h"
#include <stddef.h>
#include "myString.h"
#include <string.h>

// Forward declaration for initialize_framebuffer
extern void initialize_framebuffer(unsigned int width, unsigned int height, unsigned int bpp);

static unsigned char setup_mode_active = 0;
static unsigned char saved_cursor_visibility = 0;
static GFX_COL saved_fg_color = 0;
static GFX_COL saved_bg_color = 0;
static int saved_font_type = 0;
static unsigned char needs_redraw = 1;
static unsigned char settings_changed = 0;

// Setup menu state
static unsigned int selected_item = 0;
static unsigned int selected_baudrate_index = 0;
static unsigned int selected_keyboard_index = 0;
static unsigned int selected_fg_color = 0;
static unsigned int selected_bg_color = 0;
static unsigned int selected_font_size = 0;
static unsigned int selected_resolution_index = 0;
static unsigned int selected_cursor_blink = 1;
static const unsigned int num_setup_items = 7;

// Available baudrates
static const unsigned int available_baudrates[] = {
    9600, 19200, 38400, 57600, 115200
};
static const unsigned int num_baudrates = sizeof(available_baudrates) / sizeof(available_baudrates[0]);

// Available keyboard layouts
static const char* available_keyboards[] = {
    "us", "uk", "de", "fr"
};
static const unsigned int num_keyboards = sizeof(available_keyboards) / sizeof(available_keyboards[0]);

// Available colors
static const GFX_COL available_colors[] = {
    BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE
};
static const char* color_names[] = {
    "Black", "Red", "Green", "Yellow", "Blue", "Magenta", "Cyan", "White"
};
static const unsigned int num_colors = sizeof(available_colors) / sizeof(available_colors[0]);

// Available resolutions
static const char* available_resolutions[] = {
    "640x480", "800x600", "1024x768"
};
static const unsigned int resolution_widths[] = {
    640, 800, 1024
};
static const unsigned int resolution_heights[] = {
    480, 600, 768
};
static const unsigned int num_resolutions = sizeof(available_resolutions) / sizeof(available_resolutions[0]);

// Helper functions
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

static void draw_int_at(unsigned int row, unsigned int col, unsigned int value)
{
    char buffer[16];
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
    return num_baudrates - 1; // Default to 115200
}

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
    return 0; // Default to "us"
}

void setup_mode_enter(void)
{
    if (!setup_mode_active)
    {
        // Save current state
        gfx_term_save_cursor();
        saved_cursor_visibility = gfx_term_get_cursor_visibility();
        saved_fg_color = gfx_get_fg();
        saved_bg_color = gfx_get_bg();
        saved_font_type = font_registry_get_current_index();
        
        // Initialize setup state
        selected_item = 0;
        selected_baudrate_index = find_current_baudrate_index();
        selected_keyboard_index = find_current_keyboard_index();
        settings_changed = 0;
        
        // Hide cursor and switch to setup font
        gfx_term_set_cursor_visibility(0);
        gfx_term_set_font(0); // Use system font
        
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
        
        // Restore original state
        gfx_term_set_font(saved_font_type);
        gfx_set_fg(saved_fg_color);
        gfx_set_bg(saved_bg_color);
        gfx_term_clear_screen();
        gfx_term_restore_cursor();
        gfx_term_set_cursor_visibility(saved_cursor_visibility);
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
            if (selected_item > 0)
            {
                selected_item--;
                needs_redraw = 1;
            }
            break;
            
        case KeyDown:
            if (selected_item < num_setup_items - 1)
            {
                selected_item++;
                needs_redraw = 1;
            }
            break;
            
        case KeyLeft:
            // Handle value changes for current item
            if (selected_item == 0 && selected_baudrate_index > 0)
            {
                selected_baudrate_index--;
                settings_changed = 1;
                needs_redraw = 1;
            }
            // Add other item handlers...
            break;
            
        case KeyRight:
            // Handle value changes for current item
            if (selected_item == 0 && selected_baudrate_index < num_baudrates - 1)
            {
                selected_baudrate_index++;
                settings_changed = 1;
                needs_redraw = 1;
            }
            // Add other item handlers...
            break;
            
        case KeyEscape:
            setup_mode_exit();
            break;
            
        case KeyReturn:
            if (settings_changed)
            {
                // Apply changes
                PiGfxConfig.uartBaudrate = available_baudrates[selected_baudrate_index];
                uart_init(PiGfxConfig.uartBaudrate);
            }
            setup_mode_exit();
            break;
    }
    
    if (needs_redraw)
    {
        needs_redraw = 0;
        setup_mode_draw();
    }
}

void setup_mode_draw(void)
{
    unsigned int screen_width, screen_height;
    gfx_get_gfx_size(&screen_width, &screen_height);
    
    // Clear screen with blue background
    gfx_set_bg(BLUE);
    gfx_term_clear_screen();
    
    // Draw simple setup dialog
    unsigned int start_row = 5;
    unsigned int start_col = 10;
    
    // Title
    gfx_set_fg(YELLOW);
    gfx_set_bg(BLUE);
    draw_text_at(start_row, start_col, "PiGFX Setup");
    
    // Menu items
    for (unsigned int i = 0; i < num_setup_items; i++)
    {
        unsigned int row = start_row + 2 + i;
        
        if (i == selected_item)
        {
            gfx_set_fg(BLACK);
            gfx_set_bg(WHITE);
        }
        else
        {
            gfx_set_fg(WHITE);
            gfx_set_bg(BLUE);
        }
        
        switch (i)
        {
            case 0:
                draw_text_at(row, start_col, "Baud Rate: ");
                draw_int_at(row, start_col + 11, available_baudrates[selected_baudrate_index]);
                break;
            case 1:
                draw_text_at(row, start_col, "Keyboard:  ");
                draw_text_at(row, start_col + 11, available_keyboards[selected_keyboard_index]);
                break;
            // Add other menu items...
            default:
                draw_text_at(row, start_col, "Option");
                break;
        }
    }
    
    // Instructions
    gfx_set_fg(CYAN);
    gfx_set_bg(BLUE);
    draw_text_at(start_row + 2 + num_setup_items + 2, start_col, "Up/Down: Select  Left/Right: Change");
    draw_text_at(start_row + 2 + num_setup_items + 3, start_col, "Enter: Save  ESC: Cancel");
}