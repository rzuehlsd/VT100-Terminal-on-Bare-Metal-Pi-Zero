# VT100 Terminal Documentation

PiGFX provides a standard VT100 terminal implementation for bare-metal Raspberry Pi.

This is a pure VT100 terminal emulator that supports the standard ANSI escape sequences for terminal control.

## Standard VT100/ANSI Features

### Cursor Control
- Move cursor to position: `ESC[<row>;<col>H` or `ESC[<row>;<col>f`
- Move cursor home: `ESC[H` or `ESC[f`
- Save cursor position: `ESC[s`
- Restore cursor position: `ESC[u`
- Cursor visibility control: `ESC[?25l` (hide), `ESC[?25h` (show)

### Text Display
- Clear screen: `ESC[2J`
- Clear from cursor to end of screen: `ESC[0J` or `ESC[J`
- Clear from cursor to beginning of screen: `ESC[1J`
- Clear line: `ESC[2K`
- Clear from cursor to end of line: `ESC[0K` or `ESC[K`
- Clear from cursor to beginning of line: `ESC[1K`

### Text Attributes (SGR)
- Reset all attributes: `ESC[m` or `ESC[0m`
- Set foreground color by index: `ESC[38;5;<index>m`
- Set background color by index: `ESC[48;5;<index>m`
- Standard ANSI colors: `ESC[30-37m` (foreground), `ESC[40-47m` (background)
- Bright colors: `ESC[90-97m` (foreground), `ESC[100-107m` (background)

### Scrolling
- Standard terminal line-by-line scrolling when text reaches bottom

## Technical Details

- **Character encoding**: ASCII
- **Color support**: 8-bit indexed color (256 colors)
- **Font**: Fixed-width bitmap font
- **Screen coordinates**: Character-based positioning (rows and columns)

This implementation provides a clean, standards-compliant VT100 terminal suitable for embedded applications and retro computing projects.

## Not Supported Functions

The following VT100/ANSI escape sequences are not currently implemented:

### High Priority Missing Sequences

#### Single Character ESC Sequences (No brackets)
- `ESC c` - Reset terminal (RIS - Reset to Initial State)
- `ESC D` - Index (move cursor down, scroll if at bottom)
- `ESC E` - Next line (move to beginning of next line)
- `ESC H` - Set tab stop at current position
- `ESC M` - Reverse index (move cursor up, scroll if at top)
- `ESC Z` - Identify terminal (should respond with `ESC[?1;0c`)

#### Important Bracketed Sequences
- `ESC[<n>G` - Move cursor to column n (horizontal position absolute)
- `ESC[<n>d` - Move cursor to row n (vertical position absolute)
- `ESC[r` - Reset scroll region to full screen
- `ESC[<top>;<bottom>r` - Set scroll region (DECSTBM)
- `ESC[3g` - Clear all tab stops
- `ESC[0g` - Clear tab stop at current position

#### Device Status/Query
- `ESC[5n` - Device status report (should respond `ESC[0n`)
- `ESC[6n` - Report cursor position (should respond `ESC[<row>;<col>R`)
- `ESC[c` or `ESC[0c` - Device attributes query

#### Character Set Control
- `ESC(0` - Select character set G0 (DEC Special Character Set)
- `ESC(B` - Select character set G0 (ASCII)

### Medium Priority Missing Sequences

#### Mode Settings
- `ESC[4h` - Insert mode
- `ESC[4l` - Replace mode
- `ESC[20h` - Line feed/new line mode
- `ESC[20l` - Return mode

#### Keypad Control
- `ESC=` - Application keypad mode
- `ESC>` - Numeric keypad mode

### Low Priority Missing Sequences

#### Advanced Features
- `ESC[<n>S` - Scroll up n lines
- `ESC[<n>T` - Scroll down n lines
- `ESC[<n>X` - Erase n characters
- `ESC[<n>I` - Cursor horizontal tabulation

### Critical Limitations

1. **No single ESC sequences implemented** - Only `ESC[` sequences work
2. **Missing basic cursor positioning** (`ESC[G`, `ESC[d`)
3. **No scroll region support** - Essential for many applications
4. **No terminal identification** - Programs can't detect capabilities
5. **Missing tab handling** - No tab stops or tab movement

These missing sequences may cause compatibility issues with some terminal applications that expect full VT100 compliance.