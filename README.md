# PiZero VT100 Terminal - Enhanced Edition

A VT100 terminal emulator for Raspberry Pi Zero based on PiGFX by Filippo Bergamasco, enhanced for use with a custom 60% VT100 replica case and adapter board.

## What's Different from Original PiGFX

This enhanced version includes several improvements over the original PiGFX by Filippo Bergamasco:

### Hardware Integration

- **Custom Adapter Board**: PCB design for Raspberry Pi Zero integration into VT100 case
- **Modified Back Cover**: 3D printable back plate designed for 60% VT100 replica case
- **Optimized Pin Layout**: Connector placement adapted for compact VT100 housing

### Software Enhancements

- **Improved Configuration System**: Enhanced `pigfx.txt` file parsing with better error handling
- **Setup Dialog**: Interactive configuration menu accessible via Print Screen key
- **Font Registry**: Multiple built-in fonts with dynamic switching capability
- **Enhanced Keyboard Support**: Better PS/2 keyboard compatibility and layout options
- **Audio Features**: PWM-based bell sound and optional key click feedback
- **Debug System**: Improved logging with configurable verbosity levels

### Configuration Features

- **Runtime Configuration**: Live settings changes through setup dialog
- **Extended Options**: Additional UART, keyboard, and display settings
- **Safe Defaults**: Fallback configuration when SD card config unavailable
- **Layout Support**: Multiple keyboard layouts (US, UK, DE, etc.)

## Hardware Components

### Adapter Board

The custom PCB provides:

- Raspberry Pi Zero mounting and connections
- PS/2 keyboard connector
- UART breakout for debugging
- Power regulation and protection
- Compact form factor for VT100 case integration

### Modified Back Cover

The 3D printable back plate features:

- Mounting points for Raspberry Pi Zero
- Cutouts for power and UART connections
- Ventilation for thermal management
- Compatible with 60% VT100 replica cases

## Quick Setup

### Hardware Assembly

1. Install Raspberry Pi Zero on adapter board
2. Mount assembly in VT100 case with modified back cover
3. Connect PS/2 keyboard to adapter board
4. Connect power supply

### Software Installation

1. Format SD card as FAT32
2. Copy `kernel.img` and `pigfx.txt` to SD card root
3. Insert SD card and power on
4. Connect UART at 115200 baud for terminal access

### Configuration

- **Interactive Setup**: Press Print Screen key when keyboard connected
- **File Configuration**: Edit `pigfx.txt` on SD card for persistent settings
- **Default Settings**: System works out-of-box with reasonable defaults

## Basic Configuration (`pigfx.txt`)

```ini
# Display Settings
displayWidth = 1024
displayHeight = 768
fontSelection = 2
foregroundColor = 10
backgroundColor = 0

# UART Settings
baudrate = 115200
switchRxTx = 0

# Keyboard Settings
keyboardLayout = us
useUsbKeyboard = 1
keyboardAutorepeat = 1

# Audio Settings
soundLevel = 50
keyClick = 1
```

## Building from Source

```bash
# Build for Raspberry Pi Zero
make RPI=1

# Clean build
make clean
```

## Known Issues

### Display Frame Coverage

- **1024x768 Resolution**: First two characters may be covered by screen bezel
- **Workaround**: Avoid critical text in leftmost positions
- **Future Fix**: Logical screen offset implementation planned

## Hardware Files

- **PCB Design**: Located in `/hardware` directory (KiCad format)
- **3D Models**: OpenSCAD files for back cover modifications in `/OpenScad`
- **Assembly Guide**: See `/doc` for detailed assembly instructions

## Original Project

This project is based on [PiGFX](https://github.com/fbergama/pigfx) by Filippo Bergamasco.
The original project provides the core VT100 terminal emulation and graphics capabilities.

## License

Copyright (C) 2014-2020 Filippo Bergamasco (Original PiGFX)  
Copyright (C) 2025 Ralf Zühlsdorff (Enhanced Edition)

Licensed under the MIT License - see LICENSE file for details.

## Contributing

Contributions welcome for:

- Hardware design improvements
- Software feature enhancements  
- Documentation updates
- Testing on different Pi models
