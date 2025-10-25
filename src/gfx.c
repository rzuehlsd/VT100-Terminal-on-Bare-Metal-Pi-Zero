//
// gfx.c
// Graphic functions
//
// PiGFX is a bare metal kernel for the Raspberry Pi
// that implements a basic ANSI terminal emulator with
// the additional support of some primitive graphics functions.
// Copyright (C) 2014-2020 Filippo Bergamasco, Christian Lehner

#include "pigfx_config.h"
#include "scn_state.h"
#include "gfx.h"
#include "framebuffer.h"
#include "console.h"
#include "dma.h"
#include "utils.h"
#include "c_utils.h"
#include "timer.h"
#include "nmalloc.h"
#include "ee_printf.h"
#include "mbox.h"
#include "config.h"
#include "synchronize.h"
#include "pwm.h"

#define MIN( v1, v2 ) ( ((v1) < (v2)) ? (v1) : (v2))
#define MAX( v1, v2 ) ( ((v1) > (v2)) ? (v1) : (v2))
#define PFB( X, Y ) ( ctx.pfb + (Y) * ctx.Pitch + (X) )



int __abs__( int a )
{
    return a<0?-a:a;
}

/** Function type to compute a glyph address in a font. */
typedef unsigned char* font_fun(unsigned int c);

// Sprite support removed

/** Display properties.
 *  Holds relevant properties for the display routines. Members in this
 *  structure are updated when setting display mode or font or by getting
 *  information from DMA controller.
 */
typedef struct {
    // Graphics variables
    unsigned int W;						/// Screen pixel width
    unsigned int H;						/// Screen pixel height
    unsigned int bpp;					/// Bits depth
    unsigned int Pitch;					/// Number of bytes for one line
    unsigned int size;					/// Number of bytes in the framebuffer (double the screen size)
    unsigned char* pfb;					/// Framebuffer address
    unsigned char* pFirstFb;			/// First Framebuffer address
    unsigned char* pSecondFb;		    /// Second Framebuffer address (double buffering)
    unsigned int fb_yOffset;            /// y-Offset within the framebuffer for double buffering
    DRAWING_MODE mode;					/// Drawing mode: normal

    // Terminal variables
    struct
    {
        // Current Font variables
        unsigned char* FONT;            /// Points to font resource
        unsigned int FONTWIDTH;         /// Pixel width for characters
        unsigned int FONTHEIGHT;        /// Pixel height for characters
        unsigned int FONTCHARBYTES;     /// Number of bytes for one char in font
        unsigned int FONTWIDTH_INTS;    /// Number of 32-bits integers for font width (4 pixels / int)
        unsigned int FONTWIDTH_REMAIN;  /// Number of bytes to add to ints (when fontwidth not a multiple of 4)
        font_fun (*font_getglyph);      /// Function to get a glyph address for a character
        int FONTHEIGHT_OFFSET;          /// Y offset from BDF FONTBOUNDINGBOX (negative means descenders)

        // Current Character Display variables
        unsigned int WIDTH;				/// Terminal character width (W / font width)
        unsigned int HEIGHT;			/// Terminal character height (H / font height)
        unsigned int tab_pos;			/// 8 by default, tabulation position multiplicator
        unsigned int cursor_row;		/// Current row position (0-based)
        unsigned int cursor_col;		/// Current column position (0-based)
        unsigned int saved_cursor[2];	/// Saved cursor position
        char cursor_visible;			/// 0 if no visible cursor
        char cursor_blink;	     		/// 0 if not blinking
        unsigned int blink_timer_hnd;   /// timer handle for cursor blink

        scn_state state;				/// Current scan state
    } term;

    GFX_COL default_bg;					/// Default background characters color
    GFX_COL default_fg;					/// Default foreground characters color
    GFX_COL bg;					        /// Background characters color
    GFX_COL fg;						    /// Foreground characters color
    unsigned int reverse; 				/// reverse status: 0 - normal; 1 -reverse
    unsigned int bg32;					/// Computed ctx.bg<<24 | ctx.bg<<16 | ctx.bg<<8 | ctx.bg;
    unsigned int fg32;					/// Computed ctx.fg<<24 | ctx.fg<<16 | ctx.fg<<8 | ctx.fg;

    unsigned char* cursor_buffer;		/// Saved content under current buffer position
    unsigned int cursor_buffer_size;	/// Byte size of this buffer
    unsigned int cursor_buffer_ready;	/// 0 if buffer is empty, 1 of it stores a content

} FRAMEBUFFER_CTX;

/** Modes for ESC[=<mode>h - PC ANSI.SYS legacy */
struct DISPLAY_MODE_DEFINITION
{
    unsigned int width;
    unsigned int height;
    unsigned int bpp; 		// NB: all are actually forced to 8 bpp
};

// Number of known modes
#define LAST_MODE_NUMBER 20

/**
 * Display Modes, as they were interpreted by ANSI.SYS on PC with a CGA, EGA or VGA card
 * There is a lot of work to be done :
 * - BPP is 8 for all so all modes are actually 256 colors/pixel and 1 byte per pixel
 * - invalid modes don't do anything
 * - mode 7 (line wrapping) is ignored
 * - only 320x200 and 640*480 seem to actually modify the resolution, other modes merely limit the area on screen.
 */
static struct DISPLAY_MODE_DEFINITION ALL_MODES[LAST_MODE_NUMBER + 1] = {

        // Resolution          Corresponding PC mode    PC card
        //---------------------------------------------------

        // Legacy CGA
        {320,200,8},		// 0: text mono  40 x 25    (CGA)
        {320,200,8},		// 1: text color 40 x 25    (CGA)
        {640,480,8},		// 2: text mono  80 x 25    (CGA)
        {640,480,8},		// 3: text color 80 x 25    (CGA)
        {320,200,8},		// 4: 320 x 200 4 colors    (CGA)
        {320,200,8},		// 5: 320 x 200 mono        (CGA)
        {640,200,8},		// 6: 640 x 200 mono        (CGA)

        // Special or non assigned
        {0,0,0},			// 7: enable line wrapping
        {0,0,0},			// 8:
        {0,0,0},			// 9:
        {0,0,0},			// 10:
        {0,0,0},			// 11:
        {0,0,0},			// 12:

        // Legacy EGA
        {320,200,8},		// 13: 320 x 200 16 colors  (EGA)
        {640,200,8},		// 14: 640 x 200 16 colors  (EGA)
        {640,350,8},		// 15: 640 x 350 mono       (EGA)
        {640,350,8},		// 16: 640 x 350 16 colors  (EGA)

        // Legacy VGA
        {640,480,8},		// 17: 640 x 480 mono       (VGA)
        {640,480,8},		// 18: 640 x 480 16 colors  (VGA)
        {320,200,8},		// 19: 320 x 200 256 colors (MCGA)

        {320,240,8},		// 20: 320 x 240 256 colors (Michael Abrash X-Mode)
};

/** Forward declaration for some state functions. */
state_fun state_fun_finalletter;
state_fun state_fun_read_digit;
state_fun state_fun_selectescape;
state_fun state_fun_waitquarebracket;
state_fun state_fun_normaltext;
state_fun state_fun_ignore_digit;

#include "framebuffer.h"

// Global static to store the screen variables.
FRAMEBUFFER_CTX ctx;

// Forward declarations
void gfx_term_render_cursor();
void gfx_term_save_cursor_content();
void gfx_switch_framebuffer();

// Functions from pigfx.c called by some private sequences (set mode, debug tests ...)
extern void initialize_framebuffer(unsigned int width, unsigned int height, unsigned int bpp);

/** Generic Font function. */
unsigned char* font_get_glyph_address(unsigned int c)
{
    return (unsigned char*) ( ctx.term.FONT + c * ctx.term.FONTCHARBYTES );
}


// include the built-in fonts registration code
#include "buildin_fonts.inc"


/** Compute some font variables from font size. */
void gfx_compute_font()
{
    ctx.term.FONTCHARBYTES = ctx.term.FONTWIDTH * ctx.term.FONTHEIGHT;
    ctx.term.FONTWIDTH_INTS = ctx.term.FONTWIDTH / 4 ;
    ctx.term.FONTWIDTH_REMAIN = ctx.term.FONTWIDTH % 4;
    ctx.cursor_buffer_size = ctx.term.FONTWIDTH * ctx.term.FONTHEIGHT;
    if (ctx.cursor_buffer)
    {
        nmalloc_free(ctx.cursor_buffer);
        ctx.cursor_buffer = 0;
        ctx.cursor_buffer_ready = 0;
    }
    ctx.cursor_buffer = (unsigned char*)nmalloc_malloc(ctx.cursor_buffer_size);
    pigfx_memset(ctx.cursor_buffer, 0, ctx.cursor_buffer_size);

    // set logical terminal size
    ctx.term.WIDTH = ctx.W / ctx.term.FONTWIDTH;
    ctx.term.HEIGHT= ctx.H / ctx.term.FONTHEIGHT;
    gfx_term_save_cursor_content();
}


// Sprite collision detection removed

/** Sets the display variables. This is called by initialize_framebuffer when setting mode.
 * Default to 8x16 font if no other font was selected before.
 * @param p_framebuffer Framebuffer address as given by DMA
 * @param width Pixel width
 * @param height Pixel height
 * @param bpp Bit depth
 * @param pitch Line byte pitch as given by DMA
 * @param size Byte size for framebuffer
 */
void gfx_set_env( void* p_framebuffer, unsigned int width, unsigned int height, unsigned int bpp, unsigned int pitch, unsigned int size )
{
    dma_init();

    // Set ctx memory to 0
    pigfx_memset(&ctx, 0, sizeof(ctx));

    // set default font
    if (ctx.term.FONT == 0) {
        gfx_term_set_font(1);
    }

    // Store DMA framebuffer infos
    ctx.pFirstFb = p_framebuffer;
    ctx.pSecondFb = p_framebuffer+size/2;
    //ctx.pfb = ctx.pSecondFb;    // set invisible part of screen to start with
    ctx.pfb = ctx.pFirstFb;    // set invisible part of screen to start with
    ctx.W = width;
    ctx.H = height;
    ctx.Pitch = pitch;
    ctx.size = size/2;      // screen is only half of the framebuffer with double buffering
    ctx.bpp = bpp;

    // store terminal sizes and informations
    ctx.term.WIDTH = ctx.W / ctx.term.FONTWIDTH;
    ctx.term.HEIGHT= ctx.H / ctx.term.FONTHEIGHT;
    ctx.term.cursor_row = ctx.term.cursor_col = 0;
    ctx.term.cursor_visible = 1;
    ctx.term.state.next = state_fun_normaltext;


    // store reverse state to 'normal'
    ctx.reverse = 0;

    gfx_term_render_cursor();
}

void gfx_set_default_bg( GFX_COL col )
{
    ctx.default_bg = col;
}

void gfx_set_default_fg( GFX_COL col )
{
    ctx.default_fg = col;
}

/** Sets the background color. */
void gfx_set_bg( GFX_COL col )
{
    ctx.bg = col;
    // fill precomputed 4 bytes integer
    unsigned char* p = (unsigned char*)&ctx.bg32;
    for (size_T i = 0 ; i < sizeof(ctx.bg32) ; i++)
        *(p++) = col;
}

/** Sets the foreground color. */
void gfx_set_fg( GFX_COL col )
{
    ctx.fg = col;
    // fill precomputed 4 bytes integer
    unsigned char* p = (unsigned char*)&ctx.fg32;
    for (size_T i = 0 ; i < sizeof(ctx.fg32) ; i++)
        *(p++) = col;
}

/** Swaps the foreground and background colors. */
void gfx_swap_fg_bg()
{
    GFX_COL safe_fg = ctx.fg;
    gfx_set_fg(ctx.bg);
    gfx_set_bg(safe_fg);
}

/** Gets the current foreground color. */
GFX_COL gfx_get_fg()
{
    return ctx.fg;
}

/** Gets the current background color. */
GFX_COL gfx_get_bg()
{
    return ctx.bg;
}

/** Returns the character terminal size. */
void gfx_get_term_size( unsigned int* rows, unsigned int* cols )
{
    *rows = ctx.term.HEIGHT;
    *cols = ctx.term.WIDTH;
}

/** Returns the pixel display sizes . */
void gfx_get_gfx_size( unsigned int* width, unsigned int* height )
{
    *width = ctx.W;
    *height = ctx.H;
}

// Sprite collision detection removed

// Sprite helpers removed; keep a no-op position corrector for scroll calls
/** Sets the whole display to background color. */
void gfx_clear()
{
    // Sprites removed: nothing to clear besides framebuffer

    if (PiGfxConfig.disableGfxDMA)
    {
        unsigned int* pf = (unsigned int*)ctx.pfb;
        for (unsigned int i=0; i< ctx.size/4; i++)
        {
            pf[i] = ctx.bg32;
        }
    }
    else
    {
        // Somehow a simple memfill is not working with DMA. So we fill the first line on the screen and then use 2D mode to copy it to the rest of the screen
        unsigned int* fillScreen = (unsigned int*)ctx.pfb;
        for (unsigned int i=0; i<ctx.Pitch/4; i++)
        {
            fillScreen[i] = ctx.bg32;
        }
        dma_enqueue_operation( ctx.pfb,
                            ctx.pfb+ctx.Pitch,
                            (((ctx.H-2) & 0xFFFF )<<16) | (ctx.Pitch & 0xFFFF ), // y len << 16 | xlen
                            (-ctx.Pitch & 0xFFFF), // bits 31:16 destination stride, 15:0 source stride
                            DMA_TI_DEST_INC | DMA_TI_2DMODE | DMA_TI_SRC_INC );
        dma_execute_queue();
    }
}

/** move screen up, new bg pixels on bottom */
void gfx_scroll_down( unsigned int npixels )
{
    if (PiGfxConfig.disableGfxDMA)
    {
        for (unsigned int row = 0; row < (ctx.H - npixels); row++)
        {
            unsigned char* src = PFB(0, row + npixels);
            unsigned char* dst = PFB(0, row);
            veryfastmemcpy(dst, src, ctx.W);
        }
    }
    else
    {
        unsigned int bytes_to_copy = ctx.W * (ctx.H - npixels);
        if (bytes_to_copy > 0)
        {
            dma_memcpy_32(PFB(0, npixels), PFB(0, 0), bytes_to_copy);
        }
    }

    for (unsigned int row = ctx.H - npixels; row < ctx.H; row++)
    {
        unsigned char* pf = PFB(0, row);
        for (unsigned int col = 0; col < ctx.W; col++)
        {
            *pf++ = ctx.bg;
        }
    }
}

void gfx_scroll_up( unsigned int npixels )
{
    if (PiGfxConfig.disableGfxDMA)
    {
        for (int row = ctx.H - 1; row >= (int)npixels; row--)
        {
            unsigned char* src = PFB(0, row - npixels);
            unsigned char* dst = PFB(0, row);
            veryfastmemcpy(dst, src, ctx.W);
        }
    }
    else
    {
        unsigned int bytes_to_copy = ctx.W * (ctx.H - npixels);
        if (bytes_to_copy > 0)
        {
            for (int row = ctx.H - 1; row >= (int)npixels; row--)
            {
                dma_memcpy_32(PFB(0, row - npixels), PFB(0, row), ctx.W);
            }
        }
    }

    for (unsigned int row = 0; row < npixels; row++)
    {
        unsigned char* pf = PFB(0, row);
        for (unsigned int col = 0; col < ctx.W; col++)
        {
            *pf++ = ctx.bg;
        }
    }
}

/** move screen to the right, new bg pixels on the left */
void gfx_scroll_left( unsigned int npixels )
{
    if (npixels >= ctx.W) return;
    if (npixels == 0) return;

    unsigned char* pfb_dst;
    unsigned char* pfb_src;
    unsigned char* pfb_end;
    for (unsigned int i=0; i<ctx.H; i++)
    {
        // for all lines
        pfb_end = PFB(0, i);
        pfb_dst = PFB(ctx.W-1, i);
        pfb_src = PFB(ctx.W-1-npixels, i);
        while (pfb_src >= pfb_end)
            *pfb_dst-- = *pfb_src--;
        for (unsigned int k=0; k<npixels; k++)
            *pfb_end++ = ctx.bg;
    }
}

void gfx_scroll_right( unsigned int npixels )
{
    if (npixels >= ctx.W) return;
    if (npixels == 0) return;

    unsigned int cpPixels = ctx.W-npixels;
    for (unsigned int i=0; i<ctx.H; i++)
    {
        // for all lines
        veryfastmemcpy(PFB(0,i), PFB(npixels,i), cpPixels);
        unsigned char* pfb_bg = PFB(cpPixels, i);
        for (unsigned int j=cpPixels; j<ctx.W; j++)
        {
            *pfb_bg++ = ctx.bg;
        }
    }
}

/** draw a bg filled rectangle: */
void gfx_fill_rect( unsigned int x, unsigned int y, unsigned int width, unsigned int height )
{
    if( x >= ctx.W || y >= ctx.H )
        return;

    if( x+width > ctx.W )
        width = ctx.W-x;

    if( y+height > ctx.H )
        height = ctx.H-y;

    while( height-- )
    {
        unsigned char* pf = PFB(x,y);
        const unsigned char* const pfb_end = pf + width;

        while( pf < pfb_end )
            *pf++ = ctx.fg;
        ++y;
    }
}

/** TODO: */
void gfx_clear_rect( unsigned int x, unsigned int y, unsigned int width, unsigned int height )
{
    gfx_swap_fg_bg();
    gfx_fill_rect(x,y,width,height);
    gfx_swap_fg_bg();
}

/** Display a character at a position. The character is drawn using
 * foreground color and pixels OFF are erased using the background color.
 *  NB: Characters with codes from 0 to 31 are displayed using current font and don't have any control effect.
 *	@param row the character line number (0 = top screen)
 *	@param col the character column (0 = left)
 *	@param c the character code from 0 to 255.
 */
void gfx_putc_NORMAL( unsigned int row, unsigned int col, unsigned char c )
{
    if( col >= ctx.term.WIDTH )
        return;
    if( row >= ctx.term.HEIGHT )
        return;

    const unsigned int pixcol = col * ctx.term.FONTWIDTH;
    const unsigned int pixrow = row * ctx.term.FONTHEIGHT;

    if (ctx.term.FONTWIDTH == 8)
    {
        // optimized original code drawing 4 pixels at once using 32-bit ints
        const unsigned int FG = ctx.fg32;
        const unsigned int BG = ctx.bg32;
        const unsigned int int_stride = (ctx.Pitch>>2) - 2; // 2 ints = 8 pixels = 1 character
        register unsigned int* p_glyph = (unsigned int*)ctx.term.font_getglyph(c);
        register unsigned char h = ctx.term.FONTHEIGHT;
        register unsigned int* pf = (unsigned int*)PFB(pixcol, pixrow);
        while(h--)
        {
            // Loop unrolled for 8xH fonts
            register unsigned int gv = *p_glyph++;
            // Each byte is 00 for background pixel or FF for foreground pixel
            *pf++ =  (gv & FG) | ( ~gv & BG );
            gv = *p_glyph++;
            *pf++ =  (gv & FG) | ( ~gv & BG );
            pf += int_stride;
        }
    }
    else
    {
        // number of bytes to next line start position in framebuffer
        const unsigned int byte_stride = ctx.Pitch - ctx.term.FONTWIDTH;
        // address of character in font
        register unsigned char* p_glyph = (unsigned char*)ctx.term.font_getglyph(c);
        // copy from font to framebuffer for each font line
        register unsigned char h = ctx.term.FONTHEIGHT;
        register unsigned char* pf = (unsigned char*)PFB(pixcol, pixrow);
        while( h-- )
        {
            unsigned int w = ctx.term.FONTWIDTH;
            while( w-- )
            {
                register unsigned char gv = *p_glyph++;	// get 1 pixel of font glyph
                if (gv)
                    *pf++ = ctx.fg;
                else
                    *pf++ = ctx.bg;
            }
            pf += byte_stride;
        }
    }
}

/** Displays a character in current drawing mode. Characters with codes from 0 to 31
 *  are displayed using current font and don't have any control effect.
 *	@param row the character line number (0 = top screen)
 *	@param col the character column (0 = left)
 *	@param c the character code from 0 to 255.
 */
draw_putc_fun (*gfx_putc) = &gfx_putc_NORMAL;

/** Sets the drawing mode for character rendering.
 * @param mode the drawing mode: drawingNORMAL (only mode supported).
 */
void gfx_set_drawing_mode( DRAWING_MODE mode )
{
    ctx.mode = mode;
    // Only normal mode is supported for VT100 compatibility
    gfx_putc = gfx_putc_NORMAL;
}

/** Restore saved content under cursor.
 *
 */
void gfx_restore_cursor_content()
{
    if (!ctx.cursor_buffer_ready) return;

    unsigned char* pb = ctx.cursor_buffer;
    unsigned char* pfb = (unsigned char*)PFB( ctx.term.cursor_col * ctx.term.FONTWIDTH, ctx.term.cursor_row * ctx.term.FONTHEIGHT );
    const unsigned int byte_stride = ctx.Pitch - ctx.term.FONTWIDTH;
    unsigned int h = ctx.term.FONTHEIGHT;
    while(h--)
    {
        unsigned int w = ctx.term.FONTWIDTH;
        while (w--)
        {
            *pfb++ = *pb++;
        }
        pfb += byte_stride;
    }

    //cout("cursor restored");cout_d(ctx.term.cursor_row);cout("-");cout_d(ctx.term.cursor_col);cout_endl();
}

/** Saves framebuffer content in the cursor bufffer so t can be restored later. */
void gfx_term_save_cursor_content()
{
    unsigned char* pb = ctx.cursor_buffer;
    unsigned char* pfb = (unsigned char*)PFB(
    ctx.term.cursor_col * ctx.term.FONTWIDTH,
    ctx.term.cursor_row * ctx.term.FONTHEIGHT );
    const unsigned int byte_stride = ctx.Pitch - ctx.term.FONTWIDTH;//$$ adjust if not 8 width?
    unsigned int h = ctx.term.FONTHEIGHT;
    while(h--)
    {
        int w = ctx.term.FONTWIDTH;
        while (w--)
        {
            *pb++ = *pfb++;
        }
        pfb += byte_stride;
    }
    ctx.cursor_buffer_ready = 1;
}

/** Saves framebuffer content that is going to be replaced by the cursor and update
    the new content.
*/
void gfx_term_render_cursor()
{

    unsigned char* pb = ctx.cursor_buffer;
    //cout("pb: "); cout_h((unsigned int)pb);cout_endl();
    unsigned char* pfb = (unsigned char*)PFB(
            ctx.term.cursor_col * ctx.term.FONTWIDTH,
            ctx.term.cursor_row * ctx.term.FONTHEIGHT );
    //cout("pfb: "); cout_h((unsigned int)pfb);cout_endl();
    const unsigned int byte_stride = ctx.Pitch - ctx.term.FONTWIDTH;//$$ adjust if not 8 width?
    //cout("byte_stride: "); cout_d(byte_stride);  cout(" pitch: "); cout_d(ctx.Pitch); cout(" FONTWIDTH: "); cout_d(ctx.term.FONTWIDTH);cout_endl();
    unsigned int h = ctx.term.FONTHEIGHT;
    //cout("h: "); cout_d(h);cout_endl();

    if( ctx.term.cursor_visible )
    {
        while(h--)
        {
            unsigned int w = ctx.term.FONTWIDTH;
            while (w--)
            {
                *pb = *pfb; // Save original pixel
                if (*pfb == (ctx.fg32 & 0xFF))
                {
                    *pfb = ctx.bg32 & 0xFF;
                }
                else if (*pfb == (ctx.bg32 & 0xFF))
                {
                    *pfb = ctx.fg32 & 0xFF;
                }
                // else leave pixel unchanged
                pb++;
                pfb++;
            }
            pfb += byte_stride;
        }
        ctx.cursor_buffer_ready = 1;
    }
    else
    {
        while(h--)
        {
            unsigned int w = ctx.term.FONTWIDTH;
            while (w--)
            {
                *pfb = *pb++; // Restore original pixel
                pfb++;
            }
            pfb += byte_stride;
        }
        ctx.cursor_buffer_ready = 1;
    }
}

/** shifts content from cursor 1 character to the right */
void gfx_term_shift_right()
{
    if (PiGfxConfig.disableGfxDMA)
    {
        for (unsigned int i=0; i<ctx.term.FONTHEIGHT; i++)
        {
            unsigned int* src = (unsigned int*)PFB(ctx.W-4-ctx.term.FONTWIDTH, ctx.term.cursor_row * ctx.term.FONTHEIGHT + i);
            unsigned int* dst = (unsigned int*)PFB(ctx.W-4, ctx.term.cursor_row * ctx.term.FONTHEIGHT + i);
            unsigned int* end = (unsigned int*)PFB(ctx.term.cursor_col * ctx.term.FONTWIDTH, ctx.term.cursor_row * ctx.term.FONTHEIGHT + i);
            while (src >= end)
            {
                *dst-- = *src--;
            }
        }
    }
    else
    {
        dma_enqueue_operation( PFB((ctx.term.cursor_col) * ctx.term.FONTWIDTH, ctx.term.cursor_row * ctx.term.FONTHEIGHT),
                            PFB((ctx.term.cursor_col+1) * ctx.term.FONTWIDTH, ctx.term.cursor_row * ctx.term.FONTHEIGHT),
                            (((ctx.term.FONTHEIGHT-1) & 0xFFFF )<<16) | ((ctx.term.WIDTH-ctx.term.cursor_col-1)*ctx.term.FONTWIDTH & 0xFFFF ),
                            ((((ctx.term.cursor_col+1)*ctx.term.FONTWIDTH) & 0xFFFF)<<16 | (((ctx.term.cursor_col+1)*ctx.term.FONTWIDTH) & 0xFFFF)), /* bits 31:16 destination stride, 15:0 source stride */
                            DMA_TI_DEST_INC | DMA_TI_2DMODE | DMA_TI_SRC_INC );
        dma_execute_queue();
    }
}

/** shifts content right of cursor 1 character to the left */
void gfx_term_shift_left()
{
    if (PiGfxConfig.disableGfxDMA)
    {
        for (unsigned int i=0; i<ctx.term.FONTHEIGHT; i++)
        {
            veryfastmemcpy(PFB((ctx.term.cursor_col) * ctx.term.FONTWIDTH, ctx.term.cursor_row * ctx.term.FONTHEIGHT + i),
                           PFB((ctx.term.cursor_col+1) * ctx.term.FONTWIDTH, ctx.term.cursor_row * ctx.term.FONTHEIGHT + i),
                           (ctx.term.WIDTH-ctx.term.cursor_col)*ctx.term.FONTWIDTH);
        }
    }
    else
    {
        dma_enqueue_operation( PFB((ctx.term.cursor_col+1) * ctx.term.FONTWIDTH, ctx.term.cursor_row * ctx.term.FONTHEIGHT),
                            PFB((ctx.term.cursor_col) * ctx.term.FONTWIDTH, ctx.term.cursor_row * ctx.term.FONTHEIGHT),
                            (((ctx.term.FONTHEIGHT-1) & 0xFFFF )<<16) | ((ctx.term.WIDTH-ctx.term.cursor_col)*ctx.term.FONTWIDTH & 0xFFFF ),
                            (((ctx.term.cursor_col*ctx.term.FONTWIDTH) & 0xFFFF)<<16 | ((ctx.term.cursor_col*ctx.term.FONTWIDTH) & 0xFFFF)), /* bits 31:16 destination stride, 15:0 source stride */
                            DMA_TI_DEST_INC | DMA_TI_2DMODE | DMA_TI_SRC_INC );
        dma_execute_queue();
    }
}

/** restore cursor content
    move line content from cursor 1 position to the right
   insert blank
   redraw cursor */
void gfx_term_insert_blank()
{
    gfx_restore_cursor_content();
    gfx_term_shift_right();
    gfx_clear_rect( ctx.term.cursor_col * ctx.term.FONTWIDTH, ctx.term.cursor_row * ctx.term.FONTHEIGHT, ctx.term.FONTWIDTH, ctx.term.FONTHEIGHT );
    gfx_term_render_cursor();
}

/** move line content from right of cursor cursor 1 position to the left
    fill last character with bg
    restore cursor */
void gfx_term_delete_char()
{
    if (ctx.term.cursor_col < (ctx.term.WIDTH-1))
    {
        gfx_term_shift_left();
    }
    gfx_clear_rect( (ctx.term.WIDTH-1) * ctx.term.FONTWIDTH, ctx.term.cursor_row * ctx.term.FONTHEIGHT, ctx.term.FONTWIDTH, ctx.term.FONTHEIGHT );
    gfx_term_render_cursor();
}

/** Insert blank line at current row (shift screen down) */
void gfx_term_insert_line()
{
    unsigned int size = ctx.term.WIDTH*ctx.term.FONTWIDTH*ctx.term.FONTHEIGHT;

    gfx_restore_cursor_content();

    for(int i=ctx.term.HEIGHT-2;i>=(int)ctx.term.cursor_row; i--)
    {
        if (PiGfxConfig.disableGfxDMA)
        {
            veryfastmemcpy(PFB((0), (i+1) * ctx.term.FONTHEIGHT), PFB((0), i * ctx.term.FONTHEIGHT), size);
        }
        else
        {
            dma_memcpy_32(PFB((0), i * ctx.term.FONTHEIGHT), PFB((0), (i+1) * ctx.term.FONTHEIGHT), size);
        }
    }

    unsigned int* pos = (unsigned int*)PFB(0, ctx.term.cursor_row * ctx.term.FONTHEIGHT);
    for(unsigned int i=0; i<size/4;i++)
    {
        *pos++=ctx.bg32;
    }

    gfx_term_render_cursor();
}

// Delete the current line (shift screen up)
void gfx_term_delete_line()
{
    unsigned int size;

    if (ctx.term.cursor_row < ctx.term.HEIGHT-2)
    {
        size = ctx.term.WIDTH*ctx.term.FONTWIDTH*ctx.term.FONTHEIGHT*(ctx.term.HEIGHT-1-ctx.term.cursor_row);
        if (PiGfxConfig.disableGfxDMA)
        {
            veryfastmemcpy(PFB((0), ctx.term.cursor_row * ctx.term.FONTHEIGHT), PFB((0), (ctx.term.cursor_row+1) * ctx.term.FONTHEIGHT), size);
        }
        else
        {
            dma_memcpy_32(PFB((0), (ctx.term.cursor_row+1) * ctx.term.FONTHEIGHT), PFB((0), ctx.term.cursor_row * ctx.term.FONTHEIGHT), size);
        }
    }

    unsigned int* pos = (unsigned int*)PFB(0, (ctx.term.HEIGHT-1) * ctx.term.FONTHEIGHT);
    size = ctx.term.WIDTH*ctx.term.FONTWIDTH*ctx.term.FONTHEIGHT;
    for(unsigned int i=0; i<size/4;i++)
    {
        *pos++=ctx.bg32;
    }

    gfx_term_render_cursor();
}

/** Fill cursor buffer with the current background and framebuffer with fg.
 */
void gfx_term_render_cursor_newline()
{
    //
    const unsigned int BG = ctx.bg32; // 4 pixels
    unsigned int nwords = ctx.cursor_buffer_size / 4;
    unsigned int* pb = (unsigned int*)ctx.cursor_buffer;
    while( nwords-- )
    {
        *pb++ = BG;
    }

    if( ctx.term.cursor_visible )
        gfx_fill_rect( ctx.term.cursor_col * ctx.term.FONTWIDTH, ctx.term.cursor_row * ctx.term.FONTHEIGHT, ctx.term.FONTWIDTH, ctx.term.FONTHEIGHT );
}

void gfx_term_beep()
{
    if(!pwm800_is_active())
    {
        LogDebug("Bell %d%% %dms ON", PiGfxConfig.soundLevel, 250);
        pwm800_start((uint8_t)PiGfxConfig.soundLevel, 250);
    }
    else
    {
        pwm800_stop();

    }
        
}

/** Draws a character string and handle control characters. */
void gfx_term_putstring( const char* str )
{
    while( *str )
    {
        int checkscroll = 1;
        switch( *str )
        {
            case '\r':
                gfx_restore_cursor_content();
                ctx.term.cursor_col = 0;
                if( ctx.term.cursor_row < ctx.term.HEIGHT ) {
                    gfx_term_save_cursor_content();
                    gfx_term_render_cursor();
                }
                break;

            case '\n':
                gfx_restore_cursor_content();
                ++ctx.term.cursor_row;
                ctx.term.cursor_col = 0;
                if( ctx.term.cursor_row < ctx.term.HEIGHT ) {
                    gfx_term_save_cursor_content();
                    gfx_term_render_cursor();
                }
                break;

            case 0x09: /* tab */
                gfx_restore_cursor_content();
                ctx.term.cursor_col += 1;
                ctx.term.cursor_col =  MIN( ctx.term.cursor_col + ctx.term.tab_pos - ctx.term.cursor_col%ctx.term.tab_pos, ctx.term.WIDTH-1 );
                gfx_term_render_cursor();
                break;

            case 0x07: /* bell */
                gfx_term_beep();
                break;

            case 0x08:
            case 0x7F:
                /* backspace */
                if( ctx.term.cursor_col>0 )
                {
                    gfx_restore_cursor_content();
                    --ctx.term.cursor_col;
                    gfx_clear_rect( ctx.term.cursor_col*ctx.term.FONTWIDTH, ctx.term.cursor_row*ctx.term.FONTHEIGHT, ctx.term.FONTWIDTH, ctx.term.FONTHEIGHT );
                    gfx_term_render_cursor();
                }
                break;

            case 0xC:
                /* new page */
                gfx_term_move_cursor(0,0);
                gfx_term_clear_screen();
                break;


            default:
                checkscroll = ctx.term.state.next( *str, &(ctx.term.state) );
                break;
        }

        if( checkscroll && (ctx.term.cursor_col >= ctx.term.WIDTH ))
        {
            gfx_restore_cursor_content();
            ++ctx.term.cursor_row;
            ctx.term.cursor_col = 0;
            gfx_term_render_cursor();
        }

        if( checkscroll && (ctx.term.cursor_row >= ctx.term.HEIGHT ))
        {
            gfx_restore_cursor_content();
            --ctx.term.cursor_row;

            gfx_scroll_down(ctx.term.FONTHEIGHT);
            gfx_term_render_cursor_newline();
        }

        ++str;
    }
}


void gfx_term_set_cursor_visibility( unsigned char visible )
{
    ctx.term.cursor_visible = visible;
}

unsigned char gfx_term_get_cursor_visibility()
{
    return ctx.term.cursor_visible;
}

void gfx_term_switch_cursor_vis( __attribute__((unused)) unsigned hnd,
                                      __attribute__((unused)) void* pParam,
                                      __attribute__((unused)) void *pContext )
{
    if (ctx.term.cursor_visible)
    {
        gfx_term_set_cursor_visibility(0);
        gfx_restore_cursor_content();
    }
    else
    {
        gfx_term_set_cursor_visibility(1);
        gfx_term_render_cursor();
    }
    ctx.term.blink_timer_hnd = attach_timer_handler(2, &gfx_term_switch_cursor_vis, 0, 0);
}

void gfx_term_set_cursor_blinking( unsigned char blink )
{
    ctx.term.cursor_blink = blink;
    remove_timer(ctx.term.blink_timer_hnd);     // it's okay to be 0
    if (blink)
    {
        ctx.term.blink_timer_hnd = attach_timer_handler(2, &gfx_term_switch_cursor_vis, 0, 0);
    }
    else
    {
        // Restore any previous blinking cursor before showing static cursor
        gfx_restore_cursor_content();
        ctx.term.cursor_visible = 1;
        gfx_term_render_cursor();
    }
}


void gfx_term_move_cursor( unsigned int row, unsigned int col )
{
    // Always restore previous cursor content before moving
    gfx_restore_cursor_content();
    ctx.term.cursor_row = MIN(ctx.term.HEIGHT-1, row );
    ctx.term.cursor_col = MIN(ctx.term.WIDTH-1, col );
    // If blinking is enabled, ensure cursor is visible and buffer is up to date
    if (ctx.term.cursor_blink) {
        ctx.term.cursor_visible = 1;
    }
    gfx_term_render_cursor();
}


void gfx_term_move_cursor_d( int delta_row, int delta_col )
{
    if( (int)ctx.term.cursor_col+delta_col < 0 )
        delta_col = 0;

    if( (int)ctx.term.cursor_row+delta_row < 0 )
        delta_row = 0;
    gfx_term_move_cursor( ctx.term.cursor_row+delta_row, ctx.term.cursor_col+delta_col );
}


void gfx_term_save_cursor()
{
    ctx.term.saved_cursor[0] = ctx.term.cursor_row;
    ctx.term.saved_cursor[1] = ctx.term.cursor_col;
}


void gfx_term_restore_cursor()
{
    gfx_restore_cursor_content();
    ctx.term.cursor_row = ctx.term.saved_cursor[0];
    ctx.term.cursor_col = ctx.term.saved_cursor[1];
    gfx_term_render_cursor();
}


void gfx_term_clear_till_end()
{
    gfx_swap_fg_bg();
    gfx_fill_rect( ctx.term.cursor_col * ctx.term.FONTWIDTH, ctx.term.cursor_row * ctx.term.FONTHEIGHT, ctx.W, ctx.term.FONTHEIGHT );
    gfx_swap_fg_bg();
}

void gfx_term_clear_till_cursor()
{
    gfx_swap_fg_bg();
    gfx_fill_rect( 0, ctx.term.cursor_row * ctx.term.FONTHEIGHT, (ctx.term.cursor_col+1) * ctx.term.FONTWIDTH, ctx.term.FONTHEIGHT );
    gfx_swap_fg_bg();
    gfx_term_render_cursor();
}


void gfx_term_clear_line()
{
    gfx_swap_fg_bg();
    gfx_fill_rect( 0, ctx.term.cursor_row*ctx.term.FONTHEIGHT, ctx.W, ctx.term.FONTHEIGHT );
    gfx_swap_fg_bg();
    gfx_term_render_cursor();
}


void gfx_term_clear_screen()
{
    gfx_clear();
    gfx_term_render_cursor();
}

void gfx_term_clear_screen_from_here()
{
    if ( ctx.term.cursor_row < (ctx.term.HEIGHT-1) )
    {
        gfx_swap_fg_bg();
        gfx_fill_rect( 0, (ctx.term.cursor_row+1) * ctx.term.FONTHEIGHT, ctx.W, ctx.H );
        gfx_swap_fg_bg();
    }
    gfx_term_clear_till_end();
}

void gfx_term_clear_screen_to_here()
{
    if ( ctx.term.cursor_row > 0 )
    {
        gfx_swap_fg_bg();
        gfx_fill_rect( 0, 0, ctx.W, ctx.term.cursor_row * ctx.term.FONTHEIGHT );
        gfx_swap_fg_bg();
    }
    gfx_term_clear_till_cursor();
}


/** Sets font by type index (avoids dimension conflicts) */
void gfx_term_set_font(int font_type)
{
    const font_descriptor_t *fontInfo;
    int font = font_registry_set_by_index(font_type);
    if (font < 0) return;

    fontInfo = font_registry_get_info(font_type);

    if (fontInfo != 0)
    {
        ctx.term.FONT = (unsigned char*)fontInfo->data;
        ctx.term.FONTWIDTH = fontInfo->width;
        ctx.term.FONTHEIGHT = fontInfo->height;
        ctx.term.font_getglyph = fontInfo->get_glyph;
        gfx_compute_font();
    }
}



/** Sets the tabulation width. */
void gfx_term_set_tabulation(int width)
{
    if (width < 0) width = 8;
    if (width > (int)ctx.term.WIDTH) width = (int)ctx.term.WIDTH;
    ctx.term.tab_pos = (unsigned int)width;
}

/**  Term ANSI prefix code */
#define TERM_ESCAPE_CHAR (0x1B)

/** State parsing functions */

/** Parse the last letter in ANSI sequence.
 *  Normal ANSI escape sequences assume previous parameters are stored as numbers in state->cmd_params[].
 *
 *  state->private_mode_char can hold a character in which case the process is not
 *  following ANSI or VT100 specifications:
 *
 *      ESC[# implements graphic commands and tests (prioritary)
 *      ESC[= implements settings change: mode (PC ANSI.SYS), font, tab width
 *      ESC[? implements some ANSI commands (save/restore cursor content)
 *
 *  Any other character will end the sequence.
 *
 *  @param ch the character to scan
 *	@param state points to the current state structure
 *	@return 1 if the terminal should handle line break and screen scroll returning from this call.
 *
 */
int state_fun_final_letter( char ch, scn_state *state )
{
    int retval = 1;// handle line break and screen scroll by default
    if( state->private_mode_char == '#' )
    {
        // Non-standard graphic commands and additionam features
        switch( ch )
        {
            case '"':
                /* scroll up */
                if (state->cmd_params_size == 1)
                {
                    gfx_scroll_up(state->cmd_params[0]);
                }
                retval = 0;
            goto back_to_normal;
            break;

            case '_':
                /* scroll down */
                if (state->cmd_params_size == 1)
                {
                    gfx_scroll_down(state->cmd_params[0]);
                }
                retval = 0;
            goto back_to_normal;
            break;

            case '>':
                /* scroll right */
                if (state->cmd_params_size == 1)
                {
                    gfx_scroll_right(state->cmd_params[0]);
                }
                retval = 0;
            goto back_to_normal;
            break;

            case '<':
                /* scroll left */
                if (state->cmd_params_size == 1)
                {
                    gfx_scroll_left(state->cmd_params[0]);
                }
                retval = 0;
            goto back_to_normal;
            break;
        }
    }

    if ( state->private_mode_char == '=' )
    {
        // ANSI.SYS style mode changing
        switch( ch )
        {
        case 'h': // set resolution mode on last parameter, ignore previous
            if( state->cmd_params_size >= 1)
            {
                // parameter is the mode index in global array
                if (state->cmd_params[state->cmd_params_size - 1] <= LAST_MODE_NUMBER)
                {
                    struct DISPLAY_MODE_DEFINITION* p = & ALL_MODES[state->cmd_params[state->cmd_params_size-1]];
                    if (p->width > 0)
                    {
                        initialize_framebuffer( p->width, p->height, p->bpp);
                    }
                }
            }
            goto back_to_normal;
            break;

        case 'f': // ESC=0f choose 8x8 font, ESC=1f for 8x16, ESC=2f for 8x24
            if( state->cmd_params_size >= 1)
            {
                // parameter is the font number
                switch (state->cmd_params[state->cmd_params_size - 1])
                {
                case 0:
                    gfx_term_set_font(0);
                    break;
                case 1:
                    gfx_term_set_font(1);
                    break;
                case 2:
                    gfx_term_set_font(2);
                    break;
                default:
                    // ignore
                    break;
                }
            }
            goto back_to_normal;
            break;

        case 't': // ESC=xxxt sets tabulation width
            if( state->cmd_params_size >= 1)
            {
                gfx_term_set_tabulation(state->cmd_params[state->cmd_params_size - 1]);
            }
            goto back_to_normal;
            break;

        } // switch last letter after '=' and parameters
    } // private mode = '='

    // General 'ESC[' ANSI/VT100 commands
    switch( ch )
    {
        case 'l':
            if( state->private_mode_char == '?' &&
                state->cmd_params_size == 1 &&
                state->cmd_params[0] == 25 )
            {
                gfx_term_set_cursor_blinking(0);
                if (ctx.term.cursor_visible)
                {
                    gfx_term_set_cursor_visibility(0);
                    gfx_restore_cursor_content();
                }
            }
            goto back_to_normal;
            break;

        case 'b':
            if( state->private_mode_char == '?' &&
                state->cmd_params_size == 1 &&
                state->cmd_params[0] == 25 )
            {
                gfx_term_set_cursor_blinking(1);
            }
            goto back_to_normal;
            break;

        case 'h':
            if( state->private_mode_char == '?' &&
                state->cmd_params_size == 1 &&
                state->cmd_params[0] == 25 )
            {
                gfx_term_set_cursor_blinking(0);
                if (ctx.term.cursor_visible == 0)
                {
                    gfx_term_set_cursor_visibility(1);
                    gfx_term_render_cursor();
                }
            }
            goto back_to_normal;
            break;

        case 'K':
            if( state->cmd_params_size== 0 )
            {
                gfx_term_clear_till_end();
            }
            else if( state->cmd_params_size== 1 )
            {
                switch(state->cmd_params[0] )
                {
                    case 0:
                        gfx_term_clear_till_end();
                        goto back_to_normal;
                    case 1:
                        gfx_term_clear_till_cursor();
                        goto back_to_normal;
                    case 2:
                        gfx_term_clear_line();
                        goto back_to_normal;
                    default:
                        goto back_to_normal;
                }
            }
            goto back_to_normal;
            break;

        case 'J':
            if( state->cmd_params_size== 0 )
            {
                gfx_term_clear_screen_from_here();
            }
            else if( state->cmd_params_size== 1 )
            {
                switch(state->cmd_params[0] )
                {
                    case 0:
                        gfx_term_clear_screen_from_here();
                        goto back_to_normal;
                    case 1:
                        gfx_term_clear_screen_to_here();
                        goto back_to_normal;
                    case 2:
                        gfx_term_move_cursor(0,0);
                        gfx_term_clear_screen();
                        goto back_to_normal;
                    default:
                        goto back_to_normal;
                }
            }
            goto back_to_normal;
            break;

        case 'A':
            //if( state->cmd_params_size == 1 )
                gfx_term_move_cursor_d( -state->cmd_params[0], 0 );

            goto back_to_normal;
            break;

        case 'B':
            //if( state->cmd_params_size == 1 )
                gfx_term_move_cursor_d( state->cmd_params[0], 0 );

            goto back_to_normal;
            break;

        case 'C':
            //if( state->cmd_params_size == 1 )
                gfx_term_move_cursor_d( 0, state->cmd_params[0] );

            goto back_to_normal;
            break;

        case 'D':
            //if( state->cmd_params_size == 1 )
                gfx_term_move_cursor_d( 0, -state->cmd_params[0] );

            goto back_to_normal;
            break;

        case 'm':
            if( state->cmd_params_size == 3 &&
                state->cmd_params[0]==38 )
            {
                if (state->cmd_params[1]==5)
                {
                    // esc[38;5;colm
                    gfx_set_fg( state->cmd_params[2] );

                }
                else if (state->cmd_params[1]==6)
                {
                    // esc[38;6;colm
                    gfx_set_fg( state->cmd_params[2] );
                    gfx_set_default_fg(state->cmd_params[2]);
                }
                goto back_to_normal;
            }
            else if( state->cmd_params_size == 3 &&
                state->cmd_params[0]==48 )
            {
                if (state->cmd_params[1]==5)
                {
                    // esc[48;5;colm
                    gfx_set_bg( state->cmd_params[2] );
                }
                else if (state->cmd_params[1]==6)
                {
                    // esc[48;6;colm
                    gfx_set_bg( state->cmd_params[2] );
                    gfx_set_default_bg(state->cmd_params[2]);
                }
                goto back_to_normal;
            }
            else if (state->cmd_params_size == 0)
            {
                // esc[m
                gfx_set_bg(ctx.default_bg);
                gfx_set_fg(ctx.default_fg);
                ctx.reverse = 0; // sets reverse to 'normal' for the current defaults.
                goto back_to_normal;
            }
            else
            {
                // esc[par1;par2;par3m  (actually one to 3 params)
                if (state->cmd_params_size > 3) state->cmd_params_size = 3;     // limit to 3
                for (unsigned int i=0; i<state->cmd_params_size; i++)
                {
                    switch (state->cmd_params[i])
                    {
                        case 0:
                            // reset
                            gfx_set_bg(ctx.default_bg);
                            gfx_set_fg(ctx.default_fg);
                            ctx.reverse = 0; // sets reverse to 'normal' for the current defaults.
                            break;
                        case 1:
                            // increase intensity - as 22m for 4byte TODO: 256 Color pal
                            if (ctx.fg <= 7) gfx_set_fg(ctx.fg+8);
                            break;
                        case 2:
                           // decrease intensity -transpose dim fg colors from bright TODO 255 color pal
                           if (ctx.fg >= 8) gfx_set_fg(ctx.fg-8);
                           break;
                        case 7:
                            // toggle text mode to 'reverse'
                            if (ctx.reverse == 0)
                            {
                                gfx_swap_fg_bg();
                                ctx.reverse = 1;
                            }
                            break;
                        case 22:
                            // transpose bright fg colors from dim, this is interesting it is meant to be 'normal'
                            // but is often implemented as 'bright', this is needed for gorilla.bas compatiblity.
                            // function is fliped since the normal terminal color is often 'dim'; in this case it is.
                            // TODO: 256 color (how would this have effect?)
                            if (ctx.fg <= 7) gfx_set_fg(ctx.fg+8);
                            break;
                        case 27:
                            // toggle text mode to 'normal'
                            if (ctx.reverse == 1)
                            {
                                gfx_swap_fg_bg();
                                ctx.reverse = 0;
                            }
                        break;
                        case 30 ... 37:
                            // fg color
                            gfx_set_fg(state->cmd_params[i]-30);
                            break;
                        case 40 ... 47:
                            // bg color
                            gfx_set_bg(state->cmd_params[i]-40);
                            break;
                        case 90 ... 97:
                            // bright foreground color 8 to 15
                            gfx_set_fg(state->cmd_params[i]-82);
                            break;
                        case 100 ... 107:
                            // bright background color 8 to 15
                            gfx_set_bg(state->cmd_params[i]-92);
                            break;
                    }
                }
                goto back_to_normal;
            }
            goto back_to_normal;
            break;

        case 'f':
        case 'H':
            if( state->cmd_params_size == 2 )
            {
                int row = (state->cmd_params[0] - 1) % ctx.term.HEIGHT;
                int col = (state->cmd_params[1] - 1) % ctx.term.WIDTH; // 80 -> (79 % 80) -> 79, 81 -> (80 % 80) -> 0
                gfx_term_move_cursor(row, col);
            }
            else
                gfx_term_move_cursor(0,0);
            goto back_to_normal;
            break;

        case 's':
            gfx_term_save_cursor();
            goto back_to_normal;
            break;

        case 'u':
            gfx_term_restore_cursor();
            goto back_to_normal;
            break;

        case '@':
            // Insert a blank character position (shift line to the right)
            if( state->cmd_params_size == 1 )
            {
                gfx_term_insert_blank();
            }
            goto back_to_normal;
            break;

        case 'P':
            // Delete a character position (shift line to the left)
            if( state->cmd_params_size == 1 )
            {
                gfx_term_delete_char();
            }
            goto back_to_normal;
            break;

        case 'L':
            // Insert blank line at current row (shift screen down)
            if( state->cmd_params_size == 1 )
            {
                gfx_term_insert_line();
            }
            goto back_to_normal;
            break;

        case 'M':
            // Delete the current line (shift screen up)
            if( state->cmd_params_size == 1 )
            {
                gfx_term_delete_line();
            }
            goto back_to_normal;
            break;

        default:
            goto back_to_normal;
    }

back_to_normal:
    // go back to normal text
    state->cmd_params_size = 0;
    state->next = state_fun_normaltext;
    return retval;
}

/** Read next digit of a parameter or a separator. */
int state_fun_read_digit( char ch, scn_state *state )
{
    if( ch>='0' && ch <= '9' )
    {
        // make sure we have a parameter
        if (state->cmd_params_size == 0)
        {
            state->cmd_params_size = 1;
            state->cmd_params[0] = 0;
        }
        // parse digit
        state->cmd_params[ state->cmd_params_size - 1] = state->cmd_params[ state->cmd_params_size - 1]*10 + (ch-'0');
        state->next = state_fun_read_digit; // stay on this state
        return 1;
    }
    if (ch == '.')
    {
        state->next = state_fun_ignore_digit;
         return 1;
    }
    if( ch == ';' )
    {
        // Another param will follow
        state->cmd_params_size++;
        state->cmd_params[ state->cmd_params_size-1 ] = 0;
        state->next = state_fun_read_digit; // stay on this state
        return 1;
    }

    // not a digit, call the final state
    state_fun_final_letter( ch, state );
    return 1;
}

/** Ignore next digits of a parameter until separator. */
int state_fun_ignore_digit( char ch, scn_state *state )
{
    if( ch>='0' && ch <= '9' )
    {
        return 1;
    }
    if( ch == ';' )
    {
        // Another param will follow
        state->cmd_params_size++;
        state->cmd_params[ state->cmd_params_size-1 ] = 0;
        state->next = state_fun_read_digit; // stay on this state
        return 1;
    }
    // not a digit, end
    state_fun_final_letter( ch, state );
    return 1;
}

/** Right after ESC, start a parameter when reading a digit or select the private mode character. */
int state_fun_selectescape( char ch, scn_state *state )
{
    if( ch>='0' && ch<='9' )
    {
        // start of a digit
        state->cmd_params_size = 1;
        state->cmd_params[ 0 ] = ch-'0';
        state->next = state_fun_read_digit;
        return 1;
    }
    else
    {
        if( ch=='?' || ch=='#' || ch=='=' )
        {
            state->private_mode_char = ch;
            // A digit may follow
            state->cmd_params_size = 0;
            state->next = state_fun_read_digit;
            return 1;
        }
    }

    // Already at the final letter
    state_fun_final_letter( ch, state );
    return 1;
}

/** Right after ESC, wait for a [.
 *  The special case ESC ESC is displaying ESC character and end the sequence.
 */
int state_fun_waitsquarebracket( char ch, scn_state *state )
{
    if( ch=='[' )
    {
        state->cmd_params[0]=1;
        state->private_mode_char=0; // reset private mode char
        state->next = state_fun_selectescape;
        return 1;
    }

    if( ch==TERM_ESCAPE_CHAR ) // Double ESCAPE prints the ESC character
    {
        gfx_putc( ctx.term.cursor_row, ctx.term.cursor_col, ch );
        ++ctx.term.cursor_col;
        gfx_term_render_cursor();
    }

    state->next = state_fun_normaltext;
    return 1;
}

/** Starting state when receiving a character to display.
 *  If the character is ESC, a sequence parsing state is entered.
 *  If none of the previous happened, the character is displayed using current font.
 */
int state_fun_normaltext( char ch, scn_state *state )
{
    if( ch==TERM_ESCAPE_CHAR )
    {
        state->next = state_fun_waitsquarebracket;
        return 1;
    }

    gfx_putc( ctx.term.cursor_row, ctx.term.cursor_col, ch );
    ++ctx.term.cursor_col;
    gfx_term_render_cursor();
    return 1;
}

/** This can be used to flip the framebuffer
    Tests have showed that this is quite slow actually
    For the moment I'm going to disable this
**/
void gfx_switch_framebuffer()
{
    // Change FB write pointer
    unsigned char* showingFb = ctx.pfb;     // this fb is now not showing, but gets changed now to be showing
    if (ctx.pfb == ctx.pFirstFb) ctx.pfb = ctx.pSecondFb;
    else ctx.pfb = ctx.pFirstFb;

    // Change y offset of framebuffer to other buffer
    if (ctx.fb_yOffset == 0) ctx.fb_yOffset = ctx.H;
    else ctx.fb_yOffset = 0;
    fb_switch_framebuffer(ctx.fb_yOffset);

    // Copy all data of the now showing framebuffer part to the not showing framebuffer part
    dma_memcpy_32(showingFb, ctx.pfb, ctx.size);
}

/** Gets the size in bytes needed for a screen buffer. */
unsigned int gfx_get_screen_buffer_size()
{
    return ctx.size;
}

/** Saves the current screen content to a buffer. */
void gfx_save_screen_buffer(void* buffer)
{
    if (buffer != 0)
    {
        if (PiGfxConfig.disableGfxDMA)
        {
            // Simple memory copy
            unsigned char* src = ctx.pfb;
            unsigned char* dst = (unsigned char*)buffer;
            for (unsigned int i = 0; i < ctx.size; i++)
            {
                *dst++ = *src++;
            }
        }
        else
        {
            // Use DMA for faster copy
            dma_memcpy_32(buffer, ctx.pfb, ctx.size);
        }
    }
}

/** Restores screen content from a buffer. */
void gfx_restore_screen_buffer(void* buffer)
{
    if (buffer != 0)
    {
        if (PiGfxConfig.disableGfxDMA)
        {
            // Simple memory copy
            unsigned char* src = (unsigned char*)buffer;
            unsigned char* dst = ctx.pfb;
            for (unsigned int i = 0; i < ctx.size; i++)
            {
                *dst++ = *src++;
            }
        }
        else
        {
            // Use DMA for faster copy
            dma_memcpy_32(buffer, ctx.pfb, ctx.size);
        }
    }
}
