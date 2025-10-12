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

// gfx functions
extern void gfx_set_env( void* p_framebuffer, unsigned int width, unsigned int height, unsigned int bpp, unsigned int pitch, unsigned int size );
extern void gfx_set_default_bg( GFX_COL col );
extern void gfx_set_default_fg( GFX_COL col );
extern void gfx_set_bg( GFX_COL col );
extern void gfx_set_fg( GFX_COL col );
extern void gfx_swap_fg_bg();
extern void gfx_get_term_size( unsigned int* rows, unsigned int* cols );
extern void gfx_get_gfx_size( unsigned int* width, unsigned int* height );
extern void gfx_set_drawing_mode( DRAWING_MODE mode );
extern void gfx_set_transparent_color( GFX_COL color );

/*!
 * Fills the entire framebuffer with the background color
 */
extern void gfx_clear();

/*!
 * Fills a rectangle with the foreground color
 */
extern void gfx_fill_rect( unsigned int x, unsigned int y, unsigned int width, unsigned int height );

/*!
 * Renders a line from x0-y0 to x1-y1
 */
extern void gfx_line( int x0, int y0, int x1, int y1 );

/*!
 * Fills a rectangle with the background color
 */
extern void gfx_clear_rect( unsigned int x, unsigned int y, unsigned int width, unsigned int height );

/*!
 * Renders the character "c" at location (x,y).
 * This points to the current drawing mode function.
 */
extern draw_putc_fun (*gfx_putc);

/*!
 * Scrolls the entire framebuffer down (adding background color at the bottom)
 */
extern void gfx_scroll_down( unsigned int npixels );

/*!
 * Scrolls the entire framebuffer up (adding background color at the top)
 */
extern void gfx_scroll_up( unsigned int npixels );

// Sprite API removed.

/*!
 *  Terminal functions
 */
extern void gfx_term_putstring( const char* str );
extern void gfx_term_set_cursor_visibility( unsigned char visible );
extern void gfx_term_move_cursor( unsigned int row, unsigned int col );
extern void gfx_term_move_cursor_d( int delta_row, int delta_col );
extern void gfx_term_save_cursor();
extern void gfx_term_restore_cursor();
extern void gfx_term_clear_till_end();
extern void gfx_term_clear_till_cursor();
extern void gfx_term_clear_line();
extern void gfx_term_clear_screen();
extern void gfx_term_clear_screen_from_here();
extern void gfx_term_clear_screen_to_here();

extern void gfx_register_builtin_fonts(void);
extern void gfx_term_set_font(int font_type);
extern int gfx_term_get_font(void);

extern void gfx_term_set_font_by_type(int font_type);
extern int gfx_term_get_font_type(void);
extern void gfx_term_set_tabulation(int width);
extern GFX_COL gfx_get_fg();
extern GFX_COL gfx_get_bg();
extern unsigned char gfx_term_get_cursor_visibility();
extern void gfx_term_render_cursor();
extern unsigned int gfx_get_screen_buffer_size();
extern void gfx_save_screen_buffer(void* buffer);
extern void gfx_restore_screen_buffer(void* buffer);

// bitmap handling
extern unsigned char gfx_term_loading_bitmap();
extern void gfx_term_load_bitmap(char pixel);

// palette handling
extern unsigned char gfx_term_loading_palette();
extern void gfx_term_load_palette(char rgb);

extern void gfx_term_set_cursor_blinking(unsigned char blink);

#endif
