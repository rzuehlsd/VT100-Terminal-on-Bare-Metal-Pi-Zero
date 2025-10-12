# Terminal codes

Legend:

- 🟢 VT100/ANSI standard
- 🟡 VT102 addition
- 🔷 VT220 addition
- Unmarked: PiGFX extensions or later/non-DEC variants

Note: Sequences are shown using `<ESC>` for the 0x1B escape byte and CSI as `ESC[`.

## Cursor control

- `<ESC>[H` — Move to 0,0 🟢 VT100
- `<ESC>[f` — Move to 0,0 🟢 VT100
- `<ESC>[<Row>;<Col>H` — Move to `<Row>,<Col>` 🟢 VT100
- `<ESC>[<Row>;<Col>f` — Move to `<Row>,<Col>` 🟢 VT100
- `<ESC>[<n>A` — Move the cursor up `<n>` lines 🟢 VT100
- `<ESC>[<n>B` — Move the cursor down `<n>` lines 🟢 VT100
- `<ESC>[<n>C` — Move the cursor forward `<n>` characters 🟢 VT100
- `<ESC>[<n>D` — Move the cursor backward `<n>` characters 🟢 VT100
- `<ESC>[?25h` — Cursor visible 🔷 VT220
- `<ESC>[?25l` — Cursor invisible 🔷 VT220
- `<ESC>[?25b` — Cursor blinking
- `<ESC>[s` — Save the cursor position
- `<ESC>[u` — Move cursor to previously saved position

## Clearing text

- `<ESC>[K` — Clear from cursor to the end of the line 🟢 VT100
- `<ESC>[0K` — Clear from cursor to the end of the line 🟢 VT100
- `<ESC>[1K` — Clear from the beginning of the current line to the cursor 🟢 VT100
- `<ESC>[2K` — Clear the whole line 🟢 VT100
- `<ESC>[J` — Clear the screen from cursor 🟢 VT100
- `<ESC>[0J` — Clear the screen from cursor 🟢 VT100
- `<ESC>[1J` — Clear the screen until cursor position 🟢 VT100
- `<ESC>[2J` — Clear the screen and move the cursor to 0,0. Loaded bitmaps are kept. 🟢 VT100

## Insert / delete

- `<ESC>[1@` — Insert a blank character position (shift line to the right) 🟡 VT102
- `<ESC>[1P` — Delete a character position (shift line to the left) 🟡 VT102
- `<ESC>[1L` — Insert blank line at current row (shift screen down) 🟡 VT102
- `<ESC>[1M` — Delete the current line (shift screen up) 🟡 VT102

## Graphics (PiGFX extensions)

- `<ESC>[#<x0>;<y0>;<x1>;<y1>l` — Draw a line from `<x0>,<y0>` to `<x1>,<y1>`
- `<ESC>[#<x0>;<y0>;<w>;<h>r` — Fill a rectangle at `<x0>,<y0>` with width `<w>`, height `<h>`
- `<ESC>[#<x0>;<y0>;<w>;<h>R` — Draw a rectangle at `<x0>,<y0>` with width `<w>`, height `<h>`
- `<ESC>[#<x0>;<y0>;<r>c` — Fill a circle with center at `<x0>,<y0>` and radius `<r>`
- `<ESC>[#<x0>;<y0>;<r>C` — Draw a circle with center at `<x0>,<y0>` and radius `<r>`
- `<ESC>[#<x0>;<y0>;<x1>;<y1>;<x2>;<y2>T` — Draw a triangle `<x0>,<y0>` → `<x1>,<y1>` → `<x2>,<y2>`
- `<ESC>[#<idx>;<x>;<y>;<b>a` — Load ASCII encoded bitmap (index 0–127, width=`x`, height=`y`, base=10 or 16). Then send `x*y` values separated by `;`.
- `<ESC>[#<idx>;<x>;<y>;<b>A` — Load RLE-compressed ASCII bitmap (pairs of value;count), base=10 or 16.
- `<ESC>[#<idx>;<x>;<y>b` — Load raw binary bitmap (send `x*y` bytes).
- `<ESC>[#<idx>;<x>;<y>B` — Load RLE-compressed binary bitmap (pairs: byte,repeat).
- `<ESC>[#<idx>;<x>;<y>d` — Draw bitmap `idx` at position `x,y`.

## Scrolling (PiGFX extensions)

- `<ESC>[#<n>"` — Scroll up by `<n>` pixels, fill with background color
- `<ESC>[#<n>_` — Scroll down by `<n>` pixels, fill with background color
- `<ESC>[#<n><` — Scroll left by `<n>` pixels, fill with background color
- `<ESC>[#<n>>` — Scroll right by `<n>` pixels, fill with background color

## Settings and attributes

- `<ESC>[m` — Reset color attributes (gray on black) 🟢 VT100
- `<ESC>[<a>;<b>;<c>m` — Set display attributes (1 to 3 params) 🟢 VT100
  - `0` = Reset all attributes 🟢 VT100
  - `1` = Increase intensity (bold) 🟢 ANSI
  - `2` = Decrease intensity (dim) 🟢 ANSI
  - `7` = Reverse video 🟢 VT100
  - `22` = Normal intensity 🟢 ANSI
  - `27` = Turn off reverse 🟢 ANSI
  - `30 ... 37` — Foreground color 0..7 🟢 ANSI
  - `40 ... 47` — Background color 0..7 🟢 ANSI
  - `90 ... 97` — Bright foreground 8..15 🟢 ANSI
  - `100 ... 107` — Bright background 8..15 🟢 ANSI

- `<ESC>[38;5;<n>m` — Set foreground color to `<n>` (0–255)
- `<ESC>[38;6;<n>m` — Set foreground color to `<n>` and save as default
- `<ESC>[48;5;<n>m` — Set background color to `<n>` (0–255)
- `<ESC>[48;6;<n>m` — Set background color to `<n>` and save as default
- `<ESC>[58;5;<n>m` — Set transparent color to `<n>` (0–255)

- `<ESC>[=0m` — Reset color attributes and set normal drawing (text/bitmaps)
- `<ESC>[=1m` — Set XOR drawing (text only)
- `<ESC>[=2m` — Set transparent drawing (text only)
- `<ESC>[=0f` — Set 8×8 font
- `<ESC>[=1f` — Set TRS-80 8×16 font
- `<ESC>[=2f` — Set TRS-80 8×24 font
- `<ESC>[=<n>t` — Set tabulation width (8 by default)
- `<ESC>[=<n>h` — Change to display mode `<n>` (legacy PC ANSI.SYS) — approximate resolutions (see README_ADD)
- `<ESC>[=<n>p` — Load color palette `<n>` (0=XTerm default, 1=VGA, 2=custom, 3=C64)
- `<ESC>[=<b>;<n>p` — Set custom palette; `<n>` colors follow, base `<b>` is 10 or 16.
  - Colors are 24-bit RGB (RRGGBB). Terminate each value with `;`.
  - Example (red, green, blue): `<ESC>[=16;3pFF0000;00FF00;0000FF;`

## Controller messages

- [Sprite collision reporting removed]
