# PiZero VT100 Terminal - Enhanced Edition

A VT100 terminal emulator for Raspberry Pi Zero based on PiGFX by Filippo Bergamasco, enhanced for use with a custom 60% VT100 replica case and adapter board.

## Motivation

Main motivation was that I wanted a terminal emulation for my VT100 replica. Emulation should start within seconds, so a bare metal implementation on a Pi zero seems to be the way to go. I found the PIGFX implementation by Fillippo Bergamasco and used that as a starting point for my modifications. 

<div align="center">
  <img src="images/VT100-org.jpeg" alt="VT100 original" />
</div>

## Requirements

My list of requirements was rather short:
- support of VT100 ANSI control sequences, at least of the most important ones
- simulation of a VT100 font and possibly other alternative fonts
- support white on black, amber on black and green on black as terminal font colors
- include simulation of keyclick and bell sound
- configurable via config file at boot and with setup dialog
- design of an adapterboard to provide power (Pi and display), RS232 and custom Mini DIN6 connector for UART host connections
- dynamically switch Rx<>Tx to simulate null modem cable
- 8'' display with at least 800x640 pixel resolution (gives about 80x32 chars with VT100 font)
- provide 5VDC power for connected SBCs, like MBC2-Z80

## What's Different from Original PiGFX

I reduced the original version of PiGFX by Filippo Bergamasco to just support the VT100 features (without sprites, palette and font loading, graphics drawing features) and added support for my additional requirements.



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
- **Rx-Tx Switching**: dynamic switching of Tx <> Rx to simulate null modem cable
- **Safe Defaults**: Fallback configuration when SD card config unavailable
- **Layout Support**: Multiple keyboard layouts (US, UK, DE, etc.)

The following picture shows the setup dialog:


<div align="center">
  <img src="images/IMG_1098.jpeg" alt="setup dialog" />
</div>

## Hardware Components

### Adapter Board

The custom PCB provides:

- 5VDC / 2A switching power supply for Pi and display
- Raspberry Pi Zero mounting and connections
- PS/2 keyboard connector
- RS232 DB9 connector to interface with host
- Compact form factor for VT100 case integration

The following picture shows the prototyp of the adapter board still with linear voltage regulator. Final version will hold a switching buck regulator to reduce heat.

<div align="center">
  <img src="images/board.jpg" alt="adapter board prototype" />
</div>

### Modified Back Cover

The 3D printable back plate features:

- Mounting points for Raspberry Pi Zero adapter board
- Cutouts for power, UART connections and sd card extension cable
- Compatible with 60% VT100 replica cases

<div align="center">
  <img src="images/back_panel.png" alt="back cover with mounting support" />
</div>

## Quick Setup

### Hardware Assembly

1. Install Raspberry Pi Zero on adapter board
2. Mount assembly in VT100 case with modified back cover
3. Connect PS/2 keyboard to adapter board
4. Connect power supply

## Assembly of VT100 replica case

The case was printed on a Bamboo Lab P1S printer using the STÖ-files provided by megardi. I discovered that the different parts which have to be glued together did not always fit very well. So be prepared to use a lot of filler and do a lot sanding. It took some time but in the end the result was quite good. I used a special filler for plastics to prepare for the final painting. I discovered that the original color of the VT100 was "oyster white" and by comparing a RAL color table with a picture of the inside of a real VT100 I found that RAL 1013 fits very well.

Here are some pictures of the assembly process:

<div align="center">

| <img src="images/Case_1.jpg" alt="Case assembly step 1" width="225"/> | <img src="images/Case_2.jpg" alt="Case assembly step 2" width="310"/> |
| <img src="images/IMG_1095.jpeg" alt="Case assembly step 3" width="300"/> | <img src="images/screen.jpg" alt="Finished terminal screen" width="300"/> |

</div>



### Software Installation

1. Format SD card as FAT32
2. Copy `bootcode.bin`, `start.elf`, `kernel.img` and `pigfx.txt` to SD card root
3. Insert SD card and power on
4. Connect UART at 115200 baud for terminal access
5. Adjust terminal settings via setup dialog to your needs

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

VT100 replica in 60% size was designed by megardi (https://www.instructables.com/23-Scale-VT100-Terminal-Reproduction/).

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
