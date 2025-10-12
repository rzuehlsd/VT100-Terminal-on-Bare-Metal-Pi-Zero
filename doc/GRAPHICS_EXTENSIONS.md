# Graphics Extensions Usage

PiGFX adds a handful of non-standard but simple graphics commands on top of the ANSI/VT100 terminal. These operate on the 8-bit indexed-color framebuffer and are driven entirely via escape sequences over the UART.

This document covers:
- Drawing primitives (lines, rectangles, circles, scroll)
- Loading and blitting bitmaps (ASCII and binary, optional RLE)
- Palettes: selecting built-ins and uploading a custom palette
- Color/SGR helpers and drawing modes
- Error handling and limits

All sequences use ESC[ ... where ESC is 0x1B.

## Coordinate system and limits
- Screen coordinates are pixels, origin at top-left (0,0), x increases right, y increases down.
- Framebuffer is 8-bit indexed; valid color index range is 0..255.
- Bitmaps are stored internally in slots 0..127.
- Drawing clips to the framebuffer bounds; out-of-range is safely ignored.

## Drawing primitives (ESC-[#...)
Prefix ESC[# introduces PiGFX graphics commands (non-standard). Parameters are decimal integers separated by semicolons, ending with the final letter.

- Line: ESC[#l — params: x0;y0;x1;y1
- Filled rectangle: ESC[#r — params: x;y;width;height
- Rectangle outline: ESC[#R — params: x;y;width;height
- Circle outline: ESC[#C — params: x;y;radius
- Circle filled: ESC[#c — params: x;y;radius
- Scroll up: ESC[#" — params: pixels
- Scroll down: ESC[#_ — params: pixels
- Scroll right: ESC[#> — params: pixels
- Scroll left: ESC[#< — params: pixels

Colors used are the current foreground (fg) for drawing; background (bg) is used by some clears.

## Bitmaps
PiGFX can receive raw 8-bit indexed bitmaps into numbered slots, then blit them. Width/height are stored with the bitmap; blitting clips to screen.

There are two load modes: ASCII-encoded and binary. Both support an optional simple RLE in the load stream for compaction.

### Load bitmap (ASCII encoded)
- Start: ESC[#a for plain, ESC[#A for RLE
- Parameters (decimal): index;width;height;base
  - index: 0..127
  - width,height > 0
  - base: 10 or 16 (ASCII digits expected in that base)
- Then stream width*height pixels as ASCII numbers separated by ';'. For base=10 send decimal [0..255]; for base=16 send hex digits [00..FF].
- End by sending a final ';' after the last value. Loader stops automatically when the expected count is reached; any syntax error aborts.

RLE for 'A': pairs of values are interpreted as [pixel, repeatCount]. Internally the first byte sets the pixel, the second indicates how many pixels to write. Continue until width*height pixels have been produced. Terminate when full; a final ';' after the last emitted run is okay.

Examples:
- Start (base 16), slot 0, 32x16: ESC[0;32;16;16A
- Stream: 00;00;FF;FF;...; (repeat until 512 values if not using RLE)

### Load bitmap (binary)
- Start: ESC[#b for plain, ESC[#B for RLE
- Parameters (decimal): index;width;height
- Then stream width*height bytes (raw color indices) for 'b'.
- For 'B' (RLE), stream pairs: pixelByte followed by repeatCount. The loader expands each run until the image buffer is full.

### Blit loaded bitmap
- Draw: ESC[#d — params: index;x;y
- Operation: Copies the stored bitmap to framebuffer at (x,y). If the bitmap extends past edges, it’s clipped. Copy is a fast row copy; no transparency.

Notes:
- A slot is reallocated on each new load, old data is freed.
- After load completes, caches are cleaned to make data visible to DMA/GPU.

## Palettes
The framebuffer uses a 256-entry RGBA palette. PiGFX ships with these built-ins (see `src/palette.h`):
- 0: xterm-256color
- 1: VGA (EGA/CGA-compatible first 16)
- 2: custom (writable via upload below)
- 3: C64 (first 16 entries defined)

### Select a built-in palette
- ESC[=p — params: idx
- Example: ESC[=1p selects VGA palette.

### Upload a custom palette
- Two-step sequence:
  1) ESC[=p — params: base;count
     - base: 10 or 16 (ASCII base for numbers to follow)
     - count: number of entries to write (1..256)
  2) Stream count palette entries as ASCII numbers separated by ';'. Each entry is a 24-bit RGB value packed as 0xRRGGBB in the chosen base.
     - For base=16: e.g., FF0000;00FF00;0000FF;...
     - For base=10: e.g., 16711680;65280;255;...
- The custom palette writes into palette index 2 internally. After upload, colors take effect immediately.

Notes:
- Any syntax error during upload aborts the operation.
- You can upload fewer than 256 entries if only changing a subset; remaining entries keep prior values.

## Color and SGR helpers
- Set fg by index (no default update): ESC[38;5;idxm
- Set fg and default fg: ESC[38;6;idxm
- Set bg by index (no default update): ESC[48;5;idxm
- Set bg and default bg: ESC[48;6;idxm
- Set transparent color index: ESC[58;5;idxm
- Reset to defaults (fg/bg): ESC[m or ESC[0m
- Standard ANSI colors also supported: 30..37/40..47 and bright 90..97/100..107

## Drawing modes
- ESC[=m — params: mode
  - 0: normal (default)
  - 1: XOR
  - 2: TRANSPARENT
Affects character rendering. Bitmap blit ESC[#d always does an opaque copy.

## Fonts and resolution shortcuts
- Change font: ESC[=f — 0: 8x8, 1: 8x16, 2: 8x24
- Change mode (legacy PC ANSI.SYS mapping): ESC[=h — see supported mode indices in source; practically 320x200, 640x480 and 320x240 are used.
- Tab width: ESC[=t — single param width

## Error handling and edge cases
- If parameters are missing/invalid, the command is ignored and parser returns to normal text mode.
- Bitmap/palette loaders exit on syntax errors; partial data may remain unused.
- Bitmap indices outside 0..127 are ignored.
- All coordinates and sizes are clipped to the framebuffer.
- When loading large ASCII streams, ensure a final ';' is sent after the last value so the loader commits the last token.

## Minimal host-side examples
Pseudocode for sending a binary bitmap (slot 0, 32x32):
1) Send string "\x1B[0;32;32b"
2) Send 1024 bytes of pixel indices
3) Send string "\x1B[0;10;10d" to draw at (10,10)

Upload a custom palette of 3 entries in hex: red, green, blue:
1) Send "\x1B[=16;3p"
2) Send "FF0000;00FF00;0000FF;"

That’s it—these are stable in the current codebase (`gfx.c` and `framebuffer.c`). For more details, see inline comments near `state_fun_final_letter` and the helpers `gfx_term_load_bitmap`/`gfx_term_load_palette`.

### Helper tool

See `tools/uart_send.py` for a small Python script (pyserial) to send bitmap and palette sequences from a host machine.
