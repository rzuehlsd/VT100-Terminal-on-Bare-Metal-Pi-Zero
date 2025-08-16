# PiGFX Enhanced Edition
## Recent Updates and Features

### Enhanced Configuration System (2025)

PiGFX Enhanced Edition now features a comprehensive two-stage configuration system with advanced display management:

**Two-Stage Configuration:**
- **Stage 1**: Safe boot configuration applied early (before "Waiting for UART" message)
- **Stage 2**: User configuration applied after UART initialization for proper timing
- Prevents initialization timing issues while maintaining configurability

**Enhanced pigfx.txt Configuration:**
- Complete color palette documentation with index mapping (0-255)
- Font selection guide with all available font indices
- Resolution switching support (640x480 ↔ 1024x768)
- Comprehensive configuration examples and explanations

**Advanced Display Features:**
- Dynamic resolution switching without reboot
- Automatic framebuffer re-initialization
- Improved font registry with better error handling
- Enhanced setup dialog with 6 configuration options

**Configuration File Documentation:**
The pigfx.txt file now includes detailed documentation for:
- Color index mapping (0-255 palette reference)
- Available fonts and their indices
- Display resolution options
- All configuration parameters with examples

### Unified Build System (2025)

PiGFX Enhanced Edition features a completely redesigned build system:

**Single RPI Variable Control:**
- Use `make RPI=1` for Raspberry Pi 1
- Use `make RPI=2` for Raspberry Pi 2  
- Use `make RPI=3` for Raspberry Pi 3
- Use `make RPI=4` for Raspberry Pi 4

**Automatic Toolchain Selection:**
- Pi 1-3: Automatically uses `arm-none-eabi-` toolchain
- Pi 4: Automatically uses `aarch64-linux-gnu-` toolchain
- No manual toolchain configuration required

**Intelligent USB Library Management:**
- Automatically builds uspi library for Pi 1-3
- Skips uspi for Pi 4 (not required)
- Automatic Config.mk regeneration when switching Pi versions
- Proper cross-compilation with correct architectures

**Build Information Display:**
- Shows selected Pi model and target kernel
- Displays active toolchain
- Indicates uspi library status
- Clear build completion summary

---

## Version 1.1.1 Features

*Modifications by Francis Piérot*

This is a summarize of what I added or modified in PiGfx in 1.1.0 version.

Although I mostly want to add features in PiGfxx so I can call them from Spencer Owens RC2014 
homebrew computer, PiGfx has nothing RC2014-specific so I put in conditionnal parts the RC2014 logo display at startup, which is referenced in the binary_assets.s and pigfx.c source files, and implies the build in the "/sprite" directory.

The RC2014 stuff can be built by enabling the RC2014 symbol in pigfx_config.h.in before building. You'll
need the content of the "sprite" directory which is not part of PIGFX in a strict sense.

The fonts must be built by doing a 'make' in the '/fonts' directory before pigfx can be built.

- Extended the buildfont tool (see the [fonts](fonts/) directory)
- Added 8x16 and 8x24 TRS-80 fonts to existing 8x8 ANSI font.
- Added `ESC[=xxh` (legacy PC ANSI.SYS) to change display mode (mostly 640x480x8 and 320x200x8, see below)
- Added `ESC[=0f` for 8x8 font
- Added `ESC[=1f` for 8x16 font
- Added `ESC[=2f` for 8x24 font
- Added `ESC[=0m` for normal mode (sprites and characters)
- Added `ESC[=1m` for XOR mode (sprites and characters)
- Added `ESC[=2m` for TRANSPARENT mode (sprites and characters)
- Added `ESC[=xxt` to set tabulation width (8 by default)
- Initial display draws a map of colors.

The following have been done in C sources.

- Constants for colors 0-15 (DRAWING_COLOR enum, can be used where GFX_COL is used)
- Function pointers for drawing-mode related operations (sprite and character)
- Varous comments and function headers
- Precomputed constants based on font width and height to avoid calculations at drawing time
- Moved scn_state structure in a separate header
- The state functions return 0 if the terminal loop should ignore line breaks and screen scroll (this is for graphics)

Version 1.1.1: coordinates in the "H" escape sequence are now 1-based instead of 0-based.
 
I try to maintain a [TODO list](TODO.md). Versions are documented in [CHANGES.MD]().

Any help to implement actual display modes other than 320x200x8 and 640x480x8 would be great.


## Setting a display mode

The `ESC[=xxh` sequence sets the mode `xx`, or actually it tries to do so. The modes actually set are limited to some 
resolutions. The table below sums up what happens for each mode. The text (character) resolution depends on the font.

Although it is not necessary, it is better to set the font after the mode.

Sequence|Theorical mode|Actual resolution|font 8x8|font 8x16|font 8x24
---|---|---|---|---|---
ESC[=0h|text mono  40 x 25    (CGA) |320x200|40x25|40x12|40x8|
ESC[=1h|text color 40 x 25    (CGA) |320x200|40x25|40x12|40x8|
ESC[=2h|text mono  80 x 25    (CGA) |640x480|80x50|80x30|80x20|
ESC[=3h|text color 80 x 25    (CGA) |640x480|80x50|80x30|80x20|
ESC[=4h|320 x 200 4 colors    (CGA) |320x200|40x25|40x12|40x8|
ESC[=5h|320 x 200 mono        (CGA) |320x200|40x25|40x12|40x8|
ESC[=6h|640 x 200 mono        (CGA) |640x200|80x25|80x12|80x8|
ESC[=13h|320 x 200 16 colors  (EGA) |320x200|40x25|40x12|40x8|
ESC[=14h|640 x 200 16 colors  (EGA) |640x200|80x25|80x12|80x8|
ESC[=15h|640 x 350 mono       (EGA) |640x350|80x43|80x21|80x14|
ESC[=16h|640 x 350 16 colors  (EGA) |640x350|80x43|80x21|80x14|
ESC[=17h|640 x 480 mono       (VGA) |640x480|80x60|80x30|80x20|
ESC[=18h|640 x 480 16 colors  (VGA) |640x480|80x60|80x30|80x20|
ESC[=19h|320 x 200 256 colors (MCGA)|320x200|40x25|40x12|40x8|
ESC[=20h|320 x 240 256 colors (MCGA)|320x240|40x30|40x15|40x10|



2018 Dec 16th
Francis Pierot