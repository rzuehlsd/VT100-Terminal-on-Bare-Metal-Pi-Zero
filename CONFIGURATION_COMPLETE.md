# PiGFX Configuration Settings - Complete Implementation

## All Setup Dialog Settings Now Configurable via pigfx.txt

All settings available in the setup dialog (Print Screen key) can now be saved and loaded from the `pigfx.txt` configuration file.

### Complete Settings List

#### [UART] Section
- `baudrate = 115200` - UART interface baudrate (300-115200)

#### [Input] Section  
- `useUsbKeyboard = 1` - Enable USB keyboard support
- `keyboardLayout = de` - Keyboard layout (us, uk, it, fr, es, de, sg)
- `sendCRLF = 1` - Send CRLF instead of LF only
- `replaceLFwithCR = 0` - Send CR instead of LF
- `backspaceEcho = 0` - Auto-echo backspace character
- `skipBackspaceEcho = 0` - Skip next character after backspace
- `swapDelWithBackspace = 1` - Substitute DEL (0x7F) with BACKSPACE (0x08)
- `keyboardAutorepeat = 1` - **NEW:** Enable keyboard autorepeat

#### [Display] Section (NEW)
- `foregroundColor = 7` - **NEW:** Default foreground color (0-255, 7=gray)
- `backgroundColor = 0` - **NEW:** Default background color (0-255, 0=black)  
- `fontSelection = 0` - **NEW:** Default font selection (font registry index)

#### [General] Section
- `showRC2014Logo = 0` - Show RC2014 logo at startup
- `disableGfxDMA = 1` - Disable fast DMA memory access
- `disableCollision = 0` - Disable sprite collision detection

## Font Selection Values

The `fontSelection` setting corresponds to font registry indices:

- `0` = 8x16 System Font (default)
- `1` = 8x8 System Font
- `2` = 8x24 System Font  
- `3` = 6x12 Spleen Font
- `4` = 12x24 Spleen Font
- `5` = 16x32 Spleen Font
- `6` = 32x64 Spleen Font
- `7` = 8x16 Spleen Font

## Color Values (0-255)

Standard colors available in setup dialog:
- 0=Black, 1=DarkRed, 2=DarkGreen, 3=DarkYellow
- 4=DarkBlue, 5=DarkMagenta, 6=DarkCyan, 7=Gray
- 8=DarkGray, 9=Red, 10=Green, 11=Yellow  
- 12=Blue, 13=Magenta, 14=Cyan, 15=White

Full 256-color palette supported (0-255).

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

[Input]  
useUsbKeyboard = 1
keyboardLayout = de
sendCRLF = 1
replaceLFwithCR = 0
backspaceEcho = 0
skipBackspaceEcho = 0
swapDelWithBackspace = 1
keyboardAutorepeat = 1

[Display]
foregroundColor = 7         ; Gray foreground
backgroundColor = 0         ; Black background  
fontSelection = 4           ; 12x24 Spleen Font

[General]
showRC2014Logo = 0
disableGfxDMA = 1
disableCollision = 0
```

## Implementation Status ✅

- ✅ All 5 setup dialog settings configurable
- ✅ Autorepeat functionality configurable  
- ✅ Font selection from registry
- ✅ Color configuration (256-color support)
- ✅ Proper validation and fallbacks
- ✅ Build system integration complete
- ✅ Backward compatibility maintained
