//
// pigfx.c
// Main entry point and core system initialization
//
// PiGFX Enhanced Edition - A bare metal kernel for the Raspberry Pi
// that implements a comprehensive ANSI terminal emulator with advanced
// graphics capabilities, dual keyboard support, and sophisticated
// configuration management.
//
// This file serves as the main system entry point and orchestrates:
// - Hardware initialization (MMU, timers, framebuffer, UART)
// - Memory management and heap setup
// - Two-stage configuration system (safe boot -> user config)
// - Keyboard support (PS/2 and USB with automatic detection)
// - Advanced debug logging with bitmap-based severity filtering
// - Font registry system with multiple font support
// - Main terminal loop for ANSI escape sequence processing
//
// Key Features:
// - Unified build system supporting Pi 1-4 with automatic toolchain selection
// - Enhanced logging system with runtime debug control and automatic colors
// - USPI compatibility layer for USB keyboard support
// - Automatic configuration loading from pigfx.txt on SD card
// - Safe initialization sequence preventing display corruption
// - Full ANSI terminal emulation with graphics extensions
//
// Copyright (C) 2014-2020 Filippo Bergamasco, Christian Lehner
//                    2025 Ralf Zühlsdorff
// Enhanced Edition improvements and unified build system
//

#include "peri.h"
#include "pigfx_config.h"
#include "uart.h"
#include "utils.h"
#include "c_utils.h"
#include "timer.h"
#include "console.h"
#include "gfx.h"
#include "framebuffer.h"
#include "irq.h"
#include "dma.h"
#include "nmalloc.h"
#include "font_registry.h"
#include "ee_printf.h"
#include "debug_levels.h"
#include "prop.h"
#include "board.h"
#include "mbox.h"
#include "actled.h"
#include "emmc.h"
#include "mbr.h"
#include "fat.h"
#include "config.h"
#include "keyboard.h"
#include "ps2.h"
#include "memory.h"
#include "mmu.h"
#include "../uspi/include/uspi.h"
#include "font_registry.h"
#include "../uspi/include/uspi.h"
#include "uart.h"

#define UART_BUFFER_SIZE 16384 /* 16k */

// Direct usage of the new bitmap-based debug system
// No wrapper macros needed - use LogNotice, LogError, LogDebug, LogWarning directly

// For SUCCESS messages, we use LogDebug with manual green color
#define SUCCESS_PRINTF(...) do { \
    LogDebug(__VA_ARGS__); \
} while(0)


#define TEST_INIT

unsigned int led_status = 0;
unsigned long timer_ticks = 0;
unsigned char usbKeyboardFound = 0;
unsigned char ps2KeyboardFound = 0;
volatile unsigned int* pUART0_DR;
volatile unsigned int* pUART0_ICR;
volatile unsigned int* pUART0_IMSC;
volatile unsigned int* pUART0_FR;

volatile char* uart_buffer;
volatile char* uart_buffer_start;
volatile char* uart_buffer_end;
volatile char* uart_buffer_limit;

tPiGfxConfig PiGfxConfig;

extern unsigned int pheap_space;
extern unsigned int heap_sz;

extern unsigned char G_STARTUP_LOGO;


/**
 * @brief Heartbeat timer handler for activity LED blinking
 * 
 * This function is called periodically by the timer system to toggle the activity LED,
 * providing visual feedback that the system is running. The LED blinks at a rate
 * defined by HEARTBEAT_FREQUENCY to indicate system health.
 * 
 * @param hnd Timer handle (unused)
 * @param pParam Optional parameter pointer (unused)
 * @param pContext Optional context pointer (unused)
 * 
 * @note This function automatically re-registers itself for the next timer event
 * @note Uses global led_status to track current LED state
 */
static void _heartbeat_timer_handler( __attribute__((unused)) unsigned hnd,
                                      __attribute__((unused)) void* pParam,
                                      __attribute__((unused)) void *pContext )
{
    if( led_status )
    {
        led_set(0);
        led_status = 0;
    } else
    {
        led_set(1);
        led_status = 1;
    }
    timer_ticks++;

    attach_timer_handler( HEARTBEAT_FREQUENCY, _heartbeat_timer_handler, 0, 0 );
}



/**
 * @brief UART interrupt handler for filling the receive buffer
 * 
 * This interrupt service routine is called when UART data is received.
 * It reads all available bytes from the UART receive FIFO and stores them
 * in a circular buffer for later processing by the main terminal loop.
 * 
 * The function implements a circular buffer with automatic wraparound and
 * overflow protection by advancing the start pointer when the buffer fills.
 * 
 * @param data Optional data parameter (unused)
 * 
 * @note This function runs in interrupt context and should be fast
 * @note Uses global uart_buffer pointers for circular buffer management
 * @note Automatically clears UART interrupts after processing
 */
void uart_fill_queue( __attribute__((unused)) void* data )
{
    while( !( *pUART0_FR & 0x10 ))
    {
        *uart_buffer_end++ = (char)( *pUART0_DR & 0xFF );

        if( uart_buffer_end >= uart_buffer_limit )
           uart_buffer_end = uart_buffer;

        if( uart_buffer_end == uart_buffer_start )
        {
            uart_buffer_start++;
            if( uart_buffer_start >= uart_buffer_limit )
                uart_buffer_start = uart_buffer;
        }
    }

    /* Clear UART0 interrupts */
    *pUART0_ICR = 0xFFFFFFFF;
}


/**
 * @brief Initialize UART interrupt handling system
 * 
 * Sets up the UART for interrupt-driven reception by:
 * - Initializing the circular buffer pointers
 * - Configuring UART hardware registers for interrupt mode
 * - Enabling receive interrupts (RXIM)
 * - Registering the interrupt handler with the IRQ system
 * 
 * This enables non-blocking UART reception where incoming data is
 * automatically buffered by the interrupt handler for later processing.
 * 
 * @note Uses IRQ 57 for UART0 on Raspberry Pi
 * @note Buffer size is defined by UART_BUFFER_SIZE (16KB)
 * @note Must be called after basic UART initialization
 */
void initialize_uart_irq()
{
    uart_buffer_start = uart_buffer_end = uart_buffer;
    uart_buffer_limit = &( uart_buffer[ UART_BUFFER_SIZE ] );

    pUART0_DR   = (volatile unsigned int*)UART0_DR;
    pUART0_IMSC = (volatile unsigned int*)UART0_IMSC;
    pUART0_ICR = (volatile unsigned int*)UART0_ICR;
    pUART0_FR   = (volatile unsigned int*)UART0_FR;

    *pUART0_IMSC = (1<<4); // Masked interrupts: RXIM (See pag 188 of BCM2835 datasheet)
    *pUART0_ICR = 0xFFFFFFFF; // Clear UART0 interrupts

    irq_attach_handler( 57, uart_fill_queue, 0 );
}


/**
 * @brief Display system banner with PiGFX version and copyright
 * 
 * Displays the standardized PiGFX banner with version information and copyright
 * notice. The banner uses a blue background with yellow text and is formatted
 * consistently across all display contexts.
 * 
 * This function:
 * - Clears the screen
 * - Sets up banner colors (blue background, yellow text)
 * - Displays version and build information
 * - Displays copyright notice
 * - Restores default colors (black background, dark gray text)
 * 
 * @note Uses ANSI escape sequences for line clearing and positioning
 * @note Automatically restores colors after banner display
 * @note Safe to call at any point during initialization or runtime
 */
void display_system_banner()
{
    gfx_term_putstring( "\x1B[2J" ); // Clear screen
    gfx_term_putstring( "\n\n" ); 
    gfx_set_bg(BLUE);
    gfx_term_putstring( "\x1B[2K" ); // Render blue line at top
    LogDebug(" ===  PiGFX %d.%d.%d  ===  Build %s", PIGFX_MAJVERSION, PIGFX_MINVERSION, PIGFX_BUILDVERSION, PIGFX_VERSION );
    gfx_term_putstring( "\x1B[2K" );
    LogDebug(" Copyright (c) 2016 Filippo Bergamasco, 2018 F. Pierot, 2020 Ch. Lehner, 2025 R. Zuehlsdorff");
    gfx_term_putstring( "\x1B[2K" );
    LogDebug("\n\n");
    gfx_set_bg(BLACK);
}

/**
 * @brief Initialize framebuffer and graphics subsystem
 * 
 * Sets up the display framebuffer with specified dimensions and configures
 * the graphics environment for terminal operations. This function:
 * - Releases any existing framebuffer
 * - Allocates new framebuffer with given parameters
 * - Sets up color palette for 8-bit mode
 * - Configures graphics context (pitch, size, etc.)
 * - Sets default drawing mode, font, and tabulation
 * - Clears the screen
 * 
 * @param width  Display width in pixels
 * @param height Display height in pixels  
 * @param bpp    Bits per pixel (typically 8 for indexed color)
 * 
 * @note Sets font to 8x16 pixels by default
 * @note Sets tabulation to 8 characters
 * @note Uses normal drawing mode (not transparent)
 * @note Logs error if palette setup fails (when FRAMEBUFFER_DEBUG enabled)
 */
void initialize_framebuffer(unsigned int width, unsigned int height, unsigned int bpp)
{
    fb_release();

    unsigned char* p_fb=0;
    unsigned int fbsize;
    unsigned int pitch;

    unsigned int p_w = width;
    unsigned int p_h = height;
    unsigned int v_w = p_w;
    unsigned int v_h = p_h;

    fb_init( p_w, p_h,
             v_w, v_h,
             bpp,
             (void*)&p_fb,
             &fbsize,
             &pitch );

    if (fb_set_palette(0) != 0)
    {
#if ENABLED(FRAMEBUFFER_DEBUG)
        cout << "Set Palette failed" << "\n";
#endif
    }

    gfx_set_env( p_fb, v_w, v_h, bpp, pitch, fbsize );
}


/**
 * @brief Main terminal processing loop
 * 
 * This is the core terminal emulation loop that handles all user interaction
 * and ANSI escape sequence processing. The function:
 * 
 * 1. Applies user display configuration after safe system initialization
 * 2. Waits for initial UART data while polling timers and keyboards
 * 3. Enters the main processing loop that:
 *    - Reads characters from the UART buffer
 *    - Handles special modes (bitmap/palette loading)
 *    - Processes backspace echo skipping if enabled
 *    - Sends characters to the graphics terminal for display
 *    - Polls timers and keyboard handlers
 * 
 * This function never returns and runs the terminal until system reset.
 * 
 * @note User configuration is applied after the "Waiting for UART" message
 * @note Supports both PS/2 and USB keyboard input
 * @note Handles ANSI escape sequences through gfx_term_putstring()
 * @note Implements backspace echo suppression for better terminal experience
 */

void term_main_loop()
{
    LogDebug("Waiting for UART data (%d baud).\n",PiGfxConfig.uartBaudrate);


    /**/
    while( uart_buffer_start == uart_buffer_end )
    {
        timer_poll();       // ActLed working while waiting for data
        if (ps2KeyboardFound)
        {
            PS2KeyboardHandler();
            fUpdateKeyboardLeds(0);
        }
        else if (usbKeyboardFound) fUpdateKeyboardLeds(1);
    }
    /**/


    // Clear entire screen and position cursor at home
    gfx_term_putstring( "\x1B[2J" );

    char strb[2] = {0,0};

    while(1)
    {

        if( uart_buffer_start != uart_buffer_end )
        {
            strb[0] = *uart_buffer_start++;
            if( uart_buffer_start >= uart_buffer_limit )
                uart_buffer_start = uart_buffer;

            if (gfx_term_loading_bitmap())
            {
                gfx_term_load_bitmap(strb[0]);
            }
            else if (gfx_term_loading_palette())
            {
                gfx_term_load_palette(strb[0]);
            }
            else
            {
                if (PiGfxConfig.skipBackspaceEcho)
                {
                    if( time_microsec()-last_backspace_t > 50000 )
                        backspace_n_skip=0;

                    if( backspace_n_skip  > 0 )
                    {
                        //LogDebug("Skip %c",strb[0]);
                        strb[0]=0; // Skip this char
                        backspace_n_skip--;
                        if( backspace_n_skip == 0)
                            strb[0]=0x7F; // Add backspace instead
                    }
                }

                gfx_term_putstring( strb );
            }
        }

        uart_fill_queue(0);

        timer_poll();

        if (ps2KeyboardFound)
        {
            PS2KeyboardHandler();
            fUpdateKeyboardLeds(0);
        }
        else if (usbKeyboardFound) fUpdateKeyboardLeds(1);
    }

}

/**
 * @brief Initialize keyboard subsystem (PS/2 and USB)
 * 
 * This function initializes the keyboard input system by attempting to detect
 * and configure both PS/2 and USB keyboards. It follows a prioritized approach:
 * 1. First attempts to initialize PS/2 keyboard
 * 2. If PS/2 fails and USB is enabled, initializes USB keyboard subsystem
 * 
 * The function:
 * - Initializes PS/2 interface and checks for connected keyboard
 * - Sets up keyboard layout from configuration if PS/2 keyboard found
 * - Falls back to USB initialization on Raspberry Pi models < 4 if enabled
 * - Registers USB keyboard event handlers when USB keyboard detected
 * - Provides debug logging for each initialization step
 * 
 * @note PS/2 keyboard takes priority over USB keyboard
 * @note USB keyboard support is only available on Raspberry Pi models < 4
 * @note USB keyboard initialization requires USPi library
 * @note Keyboard layout is configured from PiGfxConfig.keyboardLayout
 * 
 * @see initPS2() for PS/2 initialization details
 * @see USPiInitialize() for USB subsystem initialization
 */
void init_keyboard(void)
{
    LogDebug("Initializing PS/2:\n");
    if (initPS2() == 0)
    {
        ps2KeyboardFound = 1;
        fInitKeyboard(PiGfxConfig.keyboardLayout);
        LogNotice("PS/2 keyboard found.\n");
    }
    else
    {
        LogDebug("PS/2 keyboard not detected.\n");
    }

#if RPI<4
    if ((PiGfxConfig.useUsbKeyboard) && (ps2KeyboardFound == 0))
    {
        LogDebug("Initializing USB:\n");

        if( USPiInitialize() )
        {
            LogDebug("Initialization OK!\n");
            LogDebug("Checking for keyboards: ");

            if ( USPiKeyboardAvailable () )
            {
                fInitKeyboard(PiGfxConfig.keyboardLayout);
                USPiKeyboardRegisterKeyStatusHandlerRaw(KeyStatusHandlerRaw);
                usbKeyboardFound = 1;
                LogNotice("USB keyboard found.\n");
            }
            else
            {
                LogDebug("USB keyboard not detected.\n");
            }
        }
        else
        {
            LogError("USB initialization failed.\n");
        }
    }
    else if (!PiGfxConfig.useUsbKeyboard)
    {
        LogDebug("USB keyboard disabled in config.\n");
    }
#endif
}



/**
 * @brief Main system entry point and initialization sequence
 * 
 * This is the primary entry function called by the boot loader after basic
 * ARM initialization. It orchestrates the complete system startup in a
 * carefully ordered sequence to ensure stable operation:
 * 
 * PHASE 1 - Critical System Setup:
 * - Clears BSS section for proper C runtime environment
 * - Initializes memory management and heap allocation
 * - Sets up UART for early logging and communication
 * - Configures debug logging system with appropriate severity levels
 * - Initializes MMU and page tables for memory protection
 * 
 * PHASE 2 - Hardware Discovery and Setup:
 * - Detects Raspberry Pi board type and revision
 * - Configures activity LED based on board type
 * - Starts timer system and heartbeat LED blinking
 * - Initializes framebuffer with safe resolution (640x480)
 * - Initializes keyboard subsystem (PS/2 and USB)
 * - Sets up font registry with built-in fonts
 * 
 * PHASE 3 - User Specific Initialization:
 * - Attempts to load user settings from pigfx.txt
 * - if loading fails, applies default configuration
 * - Prints loaded configuration to debug log
 * - Applies user configuration to display and system settings
 * - Reinitializes display if resolution changed
 * - Configures UART with user-specified baud rate
 * - Configures debug logging severity based on user settings
 * 
 * PHASE 4 - Main Terminal Loop:
 * - Enters main terminal processing loop
 * 
 * @param r0    ARM register r0 from boot loader (unused)
 * @param r1    ARM register r1 from boot loader (unused) 
 * @param atags Device tree or ATAGS from boot loader (unused)
 * 
 * @note This function never returns - system runs until reset
 * @note Uses two-stage configuration for stability and flexibility
 * @note Implements automatic keyboard detection and fallback
 * @note Provides comprehensive debug logging throughout initialization
 */
void entry_point(unsigned int r0, unsigned int r1, unsigned int *atags)
{
    unsigned int boardRevision;
    board_t raspiBoard;
    tSysRam ArmRam;

    //unused
    (void) r0;
    (void) r1;
    (void) atags;

    // PHASE 1 - Critical System Setup:

        // clear BSS
        extern unsigned char __bss_start;
        extern unsigned char _end;
        for (unsigned char *pBSS = &__bss_start; pBSS < &_end; pBSS++)
        {
            *pBSS = 0;
        }

        // Heap init
        unsigned int memSize = ARM_MEMSIZE-MEM_HEAP_START;
        nmalloc_set_memory_area( (unsigned char*)MEM_HEAP_START, memSize);

        // UART buffer allocation
        uart_buffer = (volatile char*)nmalloc_malloc( UART_BUFFER_SIZE );
        uart_init(115200);
        initialize_uart_irq();

        // Init Pagetable
        CreatePageTable(ARM_MEMSIZE);
        EnableMMU();
    

    // PHASE 2 - Hardware Discovery and Initial Setup:

        // Get informations about the board we are booting
        boardRevision = prop_revision();
        raspiBoard = board_info(boardRevision);
        prop_ARMRAM(&ArmRam);

        // Where is the Act LED?
        led_init(raspiBoard);

        // Timers and heartbeat
        timers_init();
        attach_timer_handler( HEARTBEAT_FREQUENCY, _heartbeat_timer_handler, 0, 0 );

        // Initialize font registry system BEFORE applying any display configuration
        font_registry_init();   
        gfx_register_builtin_fonts();

        // set and apply the default configuration
        setDefaultConfig();
        applyConfig();

        LogNotice("Framebuffer is initialized. Now we can print to screen!\n");

        // display_system_banner();

        LogNotice("\nBooting on Raspberry Pi %s, %s, %iMB ARM RAM\n", 
                board_model(raspiBoard.model), 
                board_processor(raspiBoard.processor), 
                ArmRam.size);

        
        // init keyboard system (PS/2 and USB)
        LogNotice("Initializing keyboard system:\n");
        init_keyboard();

        LogNotice("Hardware Discovery and Initial Setup complete.\n");
        
        

    // PHASE 3 - User Specific Initialization:

        LogNotice("Reading configuration file:\n");

        int ret = 0;
        ret = loadConfigFile();
        if(ret != 0)            // Try to load user config file
        {
            LogNotice("Could not load configuration file. Error %d.\n", ret);
            setDefaultConfig();
        }
        else
            LogNotice("Configuration loaded from file.\n");
     
        printConfig();

        // LogNotice("Setting user configuration.\n");
        applyConfig();
    

    // PHASE 4 - Setup Complete - Go to Loop:
 
        LogNotice("Initialization completed.\n");
    
        term_main_loop();
}
