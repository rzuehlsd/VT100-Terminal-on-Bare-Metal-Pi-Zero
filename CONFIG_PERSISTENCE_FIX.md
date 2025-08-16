# Configuration Persistence Fix

## Problem
When changing foreground color in the setup dialog and power cycling, the changed foreground color was not used and all text appeared in light grey instead of the configured color.

## Root Cause
The configuration values from `pigfx.txt` were being loaded correctly, but were subsequently overridden by hardcoded color values in the initialization sequence.

## Specific Issue Found
In `src/pigfx.c` around line 574-575, after the configuration was loaded and applied, the code was setting:

```c
gfx_set_drawing_mode(drawingNORMAL);
gfx_set_fg(GRAY);  // This overwrote the config foreground color!
```

## Fix Applied
Changed the hardcoded color assignments to use the configuration values:

```c
gfx_set_drawing_mode(drawingNORMAL);
gfx_set_fg(PiGfxConfig.foregroundColor);
gfx_set_bg(PiGfxConfig.backgroundColor);
```

## Configuration Application Sequence
The correct sequence now ensures all configuration settings are applied in the right order:

1. **Line 437**: `initialize_framebuffer()` - Sets initial 8x16 font (before config)
2. **Line 499**: `setDefaultConfig()` - Sets internal defaults
3. **Line 502**: `lookForConfigFile()` - Loads configuration from pigfx.txt
4. **Lines 506-509**: Apply config colors as defaults and current colors
5. **Lines 577-583**: Apply config font selection via font registry
6. **Line 591**: `term_main_loop()` - Start main terminal loop

## Verification
- ✅ Foreground color from config is preserved
- ✅ Background color from config is preserved  
- ✅ Font selection from config is preserved
- ✅ Keyboard autorepeat setting from config is preserved
- ✅ All other settings continue to work correctly

## Settings That Persist Across Power Cycles
All settings defined in `pigfx.txt` are now correctly applied and remain active:

- **[UART]**: baudrate
- **[Input]**: useUsbKeyboard, keyboardLayout, keyboardAutorepeat  
- **[Display]**: foregroundColor, backgroundColor, fontSelection
- **[General]**: showRC2014Logo, disableGfxDMA, disableCollision

## Notes
- No file writing capability was added (SD card remains read-only)
- Configuration changes in setup dialog affect only the current session
- To make setup dialog changes permanent, manually edit pigfx.txt on SD card
- The fix ensures that configuration from pigfx.txt is properly respected on every boot
