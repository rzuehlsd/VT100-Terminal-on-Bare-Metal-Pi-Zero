# PiGFX Configuration Settings - Complete Implementation

## All Setup Dialog Settings Now Configurable via pigfx.txt

All settings available in the setup dialog (Print Screen key) can now be saved and loaded from the `pigfx.txt` configuration file.

### Complete Settings List

#### [UART] Section
- `baudrate = 115200` - UART interface baudrate (300-115200)
- `switchRxTx = 0` - Swap UART RX/TX pins (0=normal, 1=swapped)

#### [Input] Section  
- `useUsbKeyboard = 1` - Enable USB keyboard support
- `keyboardLayout = de` - Keyboard layout (us, uk, it, fr, es, de, sg)
- `sendCRLF = 1` - Send CRLF instead of LF only
- `replaceLFwithCR = 0` - Replace LF with CR when transmitting
- `backspaceEcho = 0` - Auto-echo backspace character
- `skipBackspaceEcho = 0` - Skip next character after backspace
- `swapDelWithBackspace = 1` - Substitute DEL (0x7F) with BACKSPACE (0x08)
- `keyboardAutorepeat = 1` - **NEW:** Enable keyboard autorepeat
- `keyboardRepeatDelay = 500` - **NEW:** Autorepeat delay in ms (e.g., 200–1000)
- `keyboardRepeatRate = 10` - **NEW:** Autorepeat rate in Hz (e.g., 10–50)

#### [Display] Section (NEW)
- `foregroundColor = 11` - **NEW:** Default foreground color (0-255, 11=yellow)
- `backgroundColor = 0` - **NEW:** Default background color (0-255, 0=black)  
- `fontSelection = 0` - **NEW:** Default font selection (font registry index)
- `displayWidth = 1024` - **NEW:** Display width (allowed: 640, 800, 1024)
- `displayHeight = 768` - **NEW:** Display height (allowed: 480, 640, 768)
- `cursorBlink = 0` - **NEW:** Show blinking text cursor (0/1)

#### [General] Section
- `disableGfxDMA = 1` - Disable fast DMA memory access
- `disableCollision = 0` - Disable sprite collision detection
- `debugVerbosity = 2` - **NEW:** Debug verbosity (0=errors+notices, 1=+warnings, 2=+debug)
- `soundLevel = 50` - **NEW:** Beep loudness (PWM duty %, 0–100)

## Font Selection Values

`fontSelection` selects a font by its registry index. The available fonts depend on the built‑in set compiled into the firmware (see Setup dialog to preview). With the current repository state (`fonts/bin/`), indices are:

- `0` = System 8x16 (default)
- `1` = System 8x24
- `2` = VT100 10x20
- `3` = VT220 10x20

## Color Values (0-255)

Standard colors available in setup dialog:
- 0=Black, 1=DarkRed, 2=DarkGreen, 3=DarkYellow
- 4=DarkBlue, 5=DarkMagenta, 6=DarkCyan, 7=Gray
- 8=DarkGray, 9=Red, 10=Green, 11=Yellow  
- 12=Blue, 13=Magenta, 14=Cyan, 15=White

Full 256-color palette supported (0-255). Defaults: foreground 11 (Yellow), background 0 (Black).

## How It Works

1. **System Startup**: Settings are loaded from `pigfx.txt` on boot
2. **Runtime Changes**: Use Print Screen key to access setup dialog
3. **Persistence**: Changes made in setup dialog are saved to the config structure
4. **Validation**: Invalid values fallback to safe defaults

## Benefits

- **Complete Configurability**: Every setup dialog option can be pre-configured
- **Persistent Settings**: No need to reconfigure after each boot
- **Backward Compatibility**: Existing pigfx.txt files continue to work
- **Safe Defaults**: Invalid configurations fallback gracefully
- **Professional Setup**: Enterprise deployment with preconfigured settings

## Example Complete Configuration

```ini
;; Complete PiGFX configuration with all new settings

[UART]
baudrate = 115200
switchRxTx = 0

[Input]  
useUsbKeyboard = 1
keyboardLayout = de
sendCRLF = 0
replaceLFwithCR = 1
backspaceEcho = 0
skipBackspaceEcho = 0
swapDelWithBackspace = 1
keyboardAutorepeat = 1
keyboardRepeatDelay = 500
keyboardRepeatRate = 10

[Display]
foregroundColor = 11        ; Yellow foreground
backgroundColor = 0         ; Black background  
fontSelection = 0           ; System 8x16
displayWidth = 1024
displayHeight = 768
cursorBlink = 0

[General]
disableGfxDMA = 1
disableCollision = 0
debugVerbosity = 2
soundLevel = 50             ; Beep loudness (0-100)
```

## Implementation Status ✅

- ✅ All setup dialog settings configurable via `pigfx.txt`
- ✅ Keyboard autorepeat, delay and rate configurable
- ✅ Font selection through registry with built‑in fonts
- ✅ Colors and resolution configurable (with validation)
- ✅ Cursor blinking, line‑ending behavior, UART switch configurable
- ✅ Debug verbosity and sound level configurable
- ✅ Backward compatibility maintained; invalid values fall back safely
