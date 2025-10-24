//
// gfx.h
// Graphic functions
//
// PiGFX is a bare metal kernel for the Raspberry Pi
// that implements a basic ANSI terminal emulator with
// the additional support of some primitive graphics functions.
// Copyright (C) 2014-2020 Filippo Bergamasco, Christian Lehner

#ifndef _GFX_H_
#define _GFX_H_

#define DEPRICATED

#include "gfx_types.h"
#include "font_registry.h"

//==============================================================================
// Core Graphics System Functions
//==============================================================================

/*!
 * @brief Initialize the graphics environment with framebuffer parameters
 * 
 * Sets up the graphics context with the provided framebuffer configuration.
 * This must be called before any other graphics operations.
 * 
 * @param p_framebuffer Pointer to the framebuffer memory
 * @param width Screen width in pixels
 * @param height Screen height in pixels  
 * @param bpp Bits per pixel (color depth)
 * @param pitch Number of bytes per scanline (may include padding)
 * @param size Total size of framebuffer in bytes
 */
extern void gfx_set_env( void* p_framebuffer, unsigned int width, unsigned int height, unsigned int bpp, unsigned int pitch, unsigned int size );

/*!
 * @brief Set the default background color
 * 
 * Sets the default background color that will be used for clearing operations
 * and as the background for new text rendering.
 * 
 * @param col The color value to use as default background
 */
extern void gfx_set_default_bg( GFX_COL col );

/*!
 * @brief Set the default foreground color
 * 
 * Sets the default foreground color that will be used for text and line drawing
 * operations.
 * 
 * @param col The color value to use as default foreground
 */
extern void gfx_set_default_fg( GFX_COL col );

/*!
 * @brief Set the current background color
 * 
 * Changes the current background color for subsequent rendering operations.
 * This affects text background and clearing operations.
 * 
 * @param col The color value to use as current background
 */
extern void gfx_set_bg( GFX_COL col );

/*!
 * @brief Set the current foreground color
 * 
 * Changes the current foreground color for subsequent rendering operations.
 * This affects text, lines, and filled shapes.
 * 
 * @param col The color value to use as current foreground
 */
extern void gfx_set_fg( GFX_COL col );

/*!
 * @brief Swap foreground and background colors
 * 
 * Exchanges the current foreground and background colors.
 * Useful for implementing inverse video mode.
 */
extern void gfx_swap_fg_bg();

/*!
 * @brief Get the terminal size in character rows and columns
 * 
 * Calculates how many character rows and columns fit in the current screen
 * resolution using the current font dimensions.
 * 
 * @param rows Pointer to store the number of character rows
 * @param cols Pointer to store the number of character columns
 */
extern void gfx_get_term_size( unsigned int* rows, unsigned int* cols );

/*!
 * @brief Get the graphics screen size in pixels
 * 
 * Returns the current screen dimensions in pixels.
 * 
 * @param width Pointer to store the screen width in pixels
 * @param height Pointer to store the screen height in pixels
 */
extern void gfx_get_gfx_size( unsigned int* width, unsigned int* height );

/*!
 * @brief Set the drawing mode for character rendering
 * 
 * Changes how characters are rendered (normal, XOR, transparent, etc.).
 * 
 * @param mode The drawing mode to use for character rendering
 */
extern void gfx_set_drawing_mode( DRAWING_MODE mode );

/*!
 * @brief Set the transparent color for transparent drawing mode
 * 
 * Defines which color should be treated as transparent when using
 * transparent drawing mode.
 * 
 * @param color The color value to treat as transparent
 */
extern void gfx_set_transparent_color( GFX_COL color );

//==============================================================================
// Basic Drawing Functions
//==============================================================================

/*!
 * @brief Clear the entire screen with background color
 * 
 * Fills the entire framebuffer with the current background color.
 */
extern void gfx_clear();

/*!
 * @brief Fill a rectangle with the foreground color
 * 
 * Draws a filled rectangle using the current foreground color.
 * 
 * @param x Left edge X coordinate
 * @param y Top edge Y coordinate  
 * @param width Rectangle width in pixels
 * @param height Rectangle height in pixels
 */
extern void gfx_fill_rect( unsigned int x, unsigned int y, unsigned int width, unsigned int height );

/*!
 * @brief Draw a line between two points
 * 
 * Renders a line from (x0,y0) to (x1,y1) using the current foreground color.
 * 
 * @param x0 Starting point X coordinate
 * @param y0 Starting point Y coordinate
 * @param x1 Ending point X coordinate
 * @param y1 Ending point Y coordinate
 */
extern void gfx_line( int x0, int y0, int x1, int y1 );

/*!
 * @brief Clear a rectangle with the background color
 * 
 * Fills a rectangular area with the current background color.
 * 
 * @param x Left edge X coordinate
 * @param y Top edge Y coordinate
 * @param width Rectangle width in pixels
 * @param height Rectangle height in pixels
 */
extern void gfx_clear_rect( unsigned int x, unsigned int y, unsigned int width, unsigned int height );

//==============================================================================
// Character Rendering
//==============================================================================

/*!
 * @brief Function pointer for character rendering
 * 
 * Points to the current character rendering function based on the selected
 * drawing mode (normal, XOR, transparent, etc.).
 * 
 * @param x X coordinate in pixels for character placement
 * @param y Y coordinate in pixels for character placement
 * @param c The character to render
 */
extern draw_putc_fun (*gfx_putc);

//==============================================================================
// Screen Scrolling Functions
//==============================================================================

/*!
 * @brief Scroll the screen content down
 * 
 * Moves all screen content down by the specified number of pixels,
 * filling the top area with the background color.
 * 
 * @param npixels Number of pixels to scroll down
 */
extern void gfx_scroll_down( unsigned int npixels );

/*!
 * @brief Scroll the screen content up
 * 
 * Moves all screen content up by the specified number of pixels,
 * filling the bottom area with the background color.
 * 
 * @param npixels Number of pixels to scroll up
 */
extern void gfx_scroll_up( unsigned int npixels );

// Sprite API removed.

//==============================================================================
// Terminal Emulation Functions
//==============================================================================

/*!
 * @brief Output a string to the terminal
 * 
 * Renders a null-terminated string at the current cursor position,
 * interpreting ANSI escape sequences and control characters.
 * 
 * @param str The null-terminated string to output
 */
extern void gfx_term_putstring( const char* str );

/*!
 * @brief Set cursor visibility on/off
 * 
 * Controls whether the terminal cursor is visible on screen.
 * 
 * @param visible 1 to show cursor, 0 to hide cursor
 */
extern void gfx_term_set_cursor_visibility( unsigned char visible );

/*!
 * @brief Move cursor to absolute position
 * 
 * Positions the cursor at the specified row and column.
 * Coordinates are 1-based (row 1, column 1 is top-left).
 * 
 * @param row Target row (1-based)
 * @param col Target column (1-based)
 */
extern void gfx_term_move_cursor( unsigned int row, unsigned int col );

/*!
 * @brief Move cursor by relative offset
 * 
 * Moves the cursor relative to its current position.
 * Positive values move right/down, negative values move left/up.
 * 
 * @param delta_row Number of rows to move (positive=down, negative=up)
 * @param delta_col Number of columns to move (positive=right, negative=left)
 */
extern void gfx_term_move_cursor_d( int delta_row, int delta_col );

/*!
 * @brief Save current cursor position
 * 
 * Stores the current cursor position and attributes for later restoration.
 * Used by ANSI escape sequences for cursor save/restore operations.
 */
extern void gfx_term_save_cursor();

/*!
 * @brief Restore previously saved cursor position
 * 
 * Restores the cursor position and attributes that were previously saved
 * with gfx_term_save_cursor().
 */
extern void gfx_term_restore_cursor();

//==============================================================================
// Screen Clearing Functions
//==============================================================================

/*!
 * @brief Clear from cursor to end of line
 * 
 * Clears all characters from the current cursor position to the end of
 * the current line, filling with background color.
 */
extern void gfx_term_clear_till_end();

/*!
 * @brief Clear from beginning of line to cursor
 * 
 * Clears all characters from the beginning of the current line up to
 * and including the cursor position, filling with background color.
 */
extern void gfx_term_clear_till_cursor();

/*!
 * @brief Clear the entire current line
 * 
 * Clears all characters on the current line, filling with background color.
 * Cursor position is preserved.
 */
extern void gfx_term_clear_line();

/*!
 * @brief Clear the entire screen
 * 
 * Clears all screen content and moves cursor to home position (1,1).
 */
extern void gfx_term_clear_screen();

/*!
 * @brief Clear from cursor to end of screen
 * 
 * Clears all content from the current cursor position to the end of
 * the screen, filling with background color.
 */
extern void gfx_term_clear_screen_from_here();

/*!
 * @brief Clear from beginning of screen to cursor
 * 
 * Clears all content from the beginning of the screen up to and
 * including the cursor position, filling with background color.
 */
extern void gfx_term_clear_screen_to_here();

//==============================================================================
// Font Management Functions
//==============================================================================

/*!
 * @brief Register all built-in fonts with the system
 * 
 * Initializes the font registry with all available built-in fonts.
 * Must be called during system initialization.
 */
extern void gfx_register_builtin_fonts(void);

/*!
 * @brief Set the current font by registry index
 * 
 * Changes the current font to the specified font from the font registry.
 * 
 * @param font_type Index of the font in the font registry
 */
extern void gfx_term_set_font(int font_type);

/*!
 * @brief Get the current font registry index
 * 
 * Returns the index of the currently active font in the font registry.
 * 
 * @return The current font registry index
 */
extern int gfx_term_get_font(void);

/*!
 * @brief Set font by type (alias for gfx_term_set_font)
 * 
 * Alternative function name for setting the font by registry index.
 * 
 * @param font_type Index of the font in the font registry
 */
extern void gfx_term_set_font_by_type(int font_type);

/*!
 * @brief Get current font type (alias for gfx_term_get_font)
 * 
 * Alternative function name for getting the current font registry index.
 * 
 * @return The current font registry index
 */
extern int gfx_term_get_font_type(void);

/*!
 * @brief Set tab stop width
 * 
 * Configures the width of tab stops for tab character processing.
 * 
 * @param width Number of character positions per tab stop
 */
extern void gfx_term_set_tabulation(int width);

//==============================================================================
// Color Query Functions
//==============================================================================

/*!
 * @brief Get the current foreground color
 * 
 * Returns the currently active foreground color value.
 * 
 * @return The current foreground color
 */
extern GFX_COL gfx_get_fg();

/*!
 * @brief Get the current background color
 * 
 * Returns the currently active background color value.
 * 
 * @return The current background color
 */
extern GFX_COL gfx_get_bg();

//==============================================================================
// Cursor Management Functions
//==============================================================================

/*!
 * @brief Get current cursor visibility state
 * 
 * Returns whether the cursor is currently visible or hidden.
 * 
 * @return 1 if cursor is visible, 0 if hidden
 */
extern unsigned char gfx_term_get_cursor_visibility();

/*!
 * @brief Render the cursor at current position
 * 
 * Draws the cursor at the current cursor position using the appropriate
 * cursor style (block, underline, etc.).
 */
extern void gfx_term_render_cursor();

/*!
 * @brief Enable or disable cursor blinking
 * 
 * Controls whether the cursor blinks automatically or remains static.
 * 
 * @param blink 1 to enable blinking, 0 to disable blinking
 */
extern void gfx_term_set_cursor_blinking(unsigned char blink);

//==============================================================================
// Screen Buffer Management Functions
//==============================================================================

/*!
 * @brief Get the size needed for screen buffer storage
 * 
 * Returns the number of bytes required to store the entire screen content
 * for save/restore operations.
 * 
 * @return Number of bytes needed for screen buffer
 */
extern unsigned int gfx_get_screen_buffer_size();

/*!
 * @brief Save current screen content to buffer
 * 
 * Copies the entire screen content to the provided buffer for later restoration.
 * Buffer must be at least gfx_get_screen_buffer_size() bytes.
 * 
 * @param buffer Pointer to buffer for storing screen content
 */
extern void gfx_save_screen_buffer(void* buffer);

/*!
 * @brief Restore screen content from buffer
 * 
 * Restores previously saved screen content from the provided buffer.
 * Used primarily by the setup dialog system.
 * 
 * @param buffer Pointer to buffer containing saved screen content
 */
extern void gfx_restore_screen_buffer(void* buffer);

#endif
