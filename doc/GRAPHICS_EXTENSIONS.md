# Graphics Extensions Usage# Graphics Extensions Usage



PiGFX adds a handful of non-standard graphics commands on top of the standard VT100 terminal functionality.PiGFX adds a handful of non-standard graphics commands on top of the standard VT100 terminal functionality.



These operate on the 8-bit indexed-color framebuffer and are driven entirely via escape sequences over the UART.These operate on the 8-bit indexed-color framebuffer and are driven entirely via escape sequences over the UART.



This document covers:This document covers:



- Scrolling operations (up, down, left, right)- Scrolling operations (up, down, left, right)

- Font selection and resolution changes- Palettes: selecting built-ins and uploading a custom palette

- Color/SGR helpers- Font selection and resolution changes

- Error handling and limits- Color/SGR helpers

- Error handling and limits

All sequences use ESC[ ... where ESC is 0x1B.

All sequences use ESC[ ... where ESC is 0x1B.

## Coordinate system and limits

## Coordinate system and limits

- Screen coordinates are pixels, origin at top-left (0,0), x increases right, y increases down.

- Framebuffer is 8-bit indexed; valid color index range is 0..255.- Screen coordinates are pixels, origin at top-left (0,0), x increases right, y increases down.

- Drawing clips to the framebuffer bounds; out-of-range is safely ignored.- Framebuffer is 8-bit indexed; valid color index range is 0..255.

- Drawing clips to the framebuffer bounds; out-of-range is safely ignored.

## Scrolling Operations (ESC-[#...)

## Scrolling Operations (ESC-[#...)

Prefix ESC[# introduces PiGFX graphics commands (non-standard). Parameters are decimal integers separated by semicolons, ending with the final letter.

Prefix ESC[# introduces PiGFX graphics commands (non-standard). Parameters are decimal integers separated by semicolons, ending with the final letter.

- Scroll up: ESC[#" — params: pixels

- Scroll down: ESC[#_ — params: pixels  - Scroll up: ESC[#" — params: pixels

- Scroll right: ESC[#> — params: pixels- Scroll down: ESC[#_ — params: pixels  

- Scroll left: ESC[#< — params: pixels- Scroll right: ESC[#> — params: pixels

- Scroll left: ESC[#< — params: pixels

These operations scroll the entire framebuffer content by the specified number of pixels. The exposed areas are filled with the background color.

These operations scroll the entire framebuffer content by the specified number of pixels. The exposed areas are filled with the background color.

## Color and SGR helpers

## Palettes

- Set fg by index (no default update): ESC[38;5;idxm

- Set fg and default fg: ESC[38;6;idxmThe framebuffer uses a 256-entry RGBA palette. PiGFX ships with these built-ins (see `src/palette.h`):

- Set bg by index (no default update): ESC[48;5;idxm

- Set bg and default bg: ESC[48;6;idxm- 0: xterm-256color

- Reset to defaults (fg/bg): ESC[m or ESC[0m- 1: VGA (EGA/CGA-compatible first 16)

- Standard ANSI colors also supported: 30..37/40..47 and bright 90..97/100..107- 2: custom (writable via upload below)

- 3: C64 (first 16 entries defined)

## Fonts and resolution shortcuts

### Select a built-in palette

- Change font: ESC[=f — 0: 8x8, 1: 8x16, 2: 8x24

- Change mode (legacy PC ANSI.SYS mapping): ESC[=h — see supported mode indices in source; practically 320x200, 640x480 and 320x240 are used.- ESC[=p — params: idx

- Tab width: ESC[=t — single param width- Example: ESC[=1p selects VGA palette.



## Error handling and edge cases### Upload a custom palette



- If parameters are missing/invalid, the command is ignored and parser returns to normal text mode.- Two-step sequence:

- All coordinates and sizes are clipped to the framebuffer.  1) ESC[=p — params: base;count

     - base: 10 or 16 (ASCII base for numbers to follow)

That's it—these are stable in the current codebase (`gfx.c` and `framebuffer.c`). For more details, see inline comments near `state_fun_final_letter` and the terminal handling functions.     - count: number of entries to write (1..256)
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
- Reset to defaults (fg/bg): ESC[m or ESC[0m
- Standard ANSI colors also supported: 30..37/40..47 and bright 90..97/100..107

## Fonts and resolution shortcuts

- Change font: ESC[=f — 0: 8x8, 1: 8x16, 2: 8x24
- Change mode (legacy PC ANSI.SYS mapping): ESC[=h — see supported mode indices in source; practically 320x200, 640x480 and 320x240 are used.
- Tab width: ESC[=t — single param width

## Error handling and edge cases

- If parameters are missing/invalid, the command is ignored and parser returns to normal text mode.
- Palette loaders exit on syntax errors; partial data may remain unused.
- All coordinates and sizes are clipped to the framebuffer.
- When loading large ASCII streams, ensure a final ';' is sent after the last value so the loader commits the last token.

## Minimal host-side examples

Upload a custom palette of 3 entries in hex: red, green, blue:

1) Send "\x1B[=16;3p"
2) Send "FF0000;00FF00;0000FF;"

That's it—these are stable in the current codebase (`gfx.c` and `framebuffer.c`). For more details, see inline comments near `state_fun_final_letter` and the palette handling functions.

### Helper tool

See `tools/uart_send.py` for a small Python script (pyserial) to send palette sequences from a host machine.