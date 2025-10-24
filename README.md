# PiGFX Enhanced Edition - VT100 Terminal Emulator for Raspberry Pi

A bare metal kernel for the Raspberry Pi that implements a comprehensive ANSI/VT100 terminal emulator with advanced graphics capabilities, dual keyboard support, and sophisticated configuration management.

## Enhanced Edition Features

### 🎯 **Core Improvements**
- **Unified Build System**: Supports Raspberry Pi 1-4 with automatic toolchain selection
- **Enhanced Logging**: Bitmap-based debug system with runtime severity control and automatic colors
- **Memory Protection**: MMU implementation with proper page table management
- **Robust Initialization**: Two-stage configuration system preventing display corruption

### ⌨️ **Advanced Keyboard Support**
- **Dual Input Support**: PS/2 and USB keyboards with automatic detection and priority
- **USB Compatibility**: USPI integration for USB keyboard support on Pi 1-3
- **Smart Fallback**: Automatic USB initialization when PS/2 keyboard not detected
- **Configurable Layouts**: Multiple keyboard layout support through configuration

### 🎨 **Enhanced Terminal Features**
- **Font Registry System**: Multiple built-in fonts with dynamic switching
- **Advanced ANSI Support**: Full VT100 escape sequence processing
- **Color Management**: Comprehensive foreground/background color control
- **Setup Dialog**: Interactive configuration interface (Print Screen key)
- **Screen Buffer Management**: Save/restore functionality for setup mode

### 🔧 **Configuration System**
- **File-Based Config**: Automatic loading from `pigfx.txt` on SD card
- **Runtime Reconfiguration**: Setup dialog for live configuration changes
- **Safe Defaults**: Fallback configuration when file loading fails
- **Comprehensive Options**: Display, keyboard, UART, and audio settings

### 🔊 **Audio Features**
- **Bell Sound**: PWM-based buzzer for terminal bell (BEL character)
- **Key Click**: Optional audible feedback for keyboard input
- **Volume Control**: Configurable sound levels (0-100%)
- **Setup Integration**: Audio settings configurable through setup dialog

### 🖥️ **Display Features**
- **Multiple Resolutions**: Support for various display modes
- **Font Scaling**: Multiple font sizes with crisp rendering
- **Cursor Control**: Configurable cursor visibility and blinking
- **Screen Management**: Advanced clearing and scrolling operations

## Hardware Compatibility

### Supported Raspberry Pi Models
- **Raspberry Pi 1 (A, A+, B, B+)**: Full support including USB keyboards
- **Raspberry Pi 2**: Full support with enhanced performance
- **Raspberry Pi 3**: Full support with 64-bit capability
- **Raspberry Pi 4**: Display and PS/2 support (USB keyboards not yet supported)
- **Raspberry Pi Zero/Zero W**: Optimized for minimal hardware

### Keyboard Support
- **PS/2 Keyboards**: Native support via GPIO bit-banging
- **USB Keyboards**: Support on Pi 1-3 via USPI library
- **Automatic Detection**: Priority-based detection (PS/2 first, then USB)
- **Layout Support**: US, UK, DE, and other international layouts

## Quick Start

### Installation
1. **Download**: Get the latest release from the releases page
2. **SD Card Setup**: Format SD card as FAT32
3. **Copy Files**: Copy `kernel*.img` and `pigfx.txt` to SD card root
4. **Hardware**: Connect UART (115200 baud) and optional PS/2 keyboard
5. **Boot**: Insert SD card and power on

### Basic UART Connection
```
Pi GPIO 14 (TX) -> Host RX
Pi GPIO 15 (RX) -> Host TX  
Pi Ground       -> Host Ground
```

### Optional PS/2 Keyboard
```
Pi GPIO 4  -> PS/2 Clock
Pi GPIO 17 -> PS/2 Data
Pi 5V      -> PS/2 VCC
Pi Ground  -> PS/2 Ground
```

## Configuration

### Configuration File (`pigfx.txt`)
```ini
# Display Settings
resolution = 4          ; 0=640x480, 1=800x600, 2=1024x768, 3=1280x720, 4=1280x1024
font = 0               ; Font index from registry
foregroundColor = 7    ; White text
backgroundColor = 0    ; Black background

# UART Settings  
uartBaudrate = 115200  ; Serial communication speed
switchRxTx = 0         ; 0=normal, 1=swap RX/TX pins

# Keyboard Settings
keyboardLayout = 0     ; 0=US, 1=UK, 2=DE, etc.
useUsbKeyboard = 1     ; Enable USB keyboard support
autoRepeat = 1         ; Enable key auto-repeat
repeatDelay = 500      ; Auto-repeat delay (ms)
repeatRate = 20        ; Auto-repeat rate (Hz)

# Audio Settings
soundLevel = 50        ; Volume level (0-100%)
keyClick = 0           ; Key click sound (0=off, 1=on)

# Terminal Settings
cursorBlink = 1        ; Cursor blinking (0=off, 1=on)
sendCRLF = 0          ; Send CR+LF for Enter key
replaceLFCR = 0       ; Replace LF with CR
```

### Interactive Setup
- **Access**: Press **Print Screen** key when keyboard connected
- **Navigation**: Use arrow keys to select options
- **Modification**: Left/Right arrows to change values
- **Save**: Press Enter to save configuration
- **Exit**: Press Escape to exit without saving

## Advanced Features

### Memory Management
- **MMU Protection**: Page table-based memory protection
- **Heap Management**: Dynamic memory allocation with nmalloc
- **Buffer Management**: Circular UART buffer (16KB) with overflow protection

### Debug System
- **Severity Levels**: Notice, Error, Debug, Warning with bitmap filtering
- **Runtime Control**: Configurable debug output levels
- **Color Coding**: Automatic color assignment for different log levels
- **Performance**: Minimal overhead when debug disabled

### Font System
- **Registry-Based**: Centralized font management system
- **Multiple Fonts**: Various sizes and styles available
- **Runtime Switching**: Dynamic font changes through setup dialog
- **Crisp Rendering**: Pixel-perfect character alignment

### UART Enhancements
- **Interrupt-Driven**: Non-blocking receive with IRQ handling
- **Pin Switching**: Optional RX/TX pin swapping via GPIO16
- **Flush Protection**: Safe buffer clearing during pin switching
- **Error Recovery**: Automatic error condition clearing

## Building from Source

### Prerequisites
```bash
# Install required toolchains
sudo apt-get install gcc-arm-linux-gnueabihf  # For Pi 1-3
sudo apt-get install gcc-aarch64-linux-gnu    # For Pi 4
```

### Build Commands
```bash
# Build for specific Pi model
make RPI=1    # Raspberry Pi 1
make RPI=2    # Raspberry Pi 2  
make RPI=3    # Raspberry Pi 3
make RPI=4    # Raspberry Pi 4

# Clean build
make clean

# Build all models
make all
```

### Build System Features
- **Automatic Toolchain Selection**: Chooses correct compiler for target
- **Dependency Tracking**: Incremental builds with proper dependencies
- **Debug Control**: Configurable debug levels at compile time
- **Model-Specific Optimization**: Tailored builds for each Pi generation

## Technical Architecture

### System Initialization
1. **Boot Loader**: ARM initialization and basic setup
2. **BSS Clearing**: C runtime environment preparation
3. **Memory Setup**: Heap initialization and MMU configuration
4. **Hardware Discovery**: Board detection and peripheral initialization
5. **Configuration Loading**: User settings from SD card
6. **Subsystem Init**: Graphics, fonts, keyboards, audio
7. **Main Loop**: Terminal processing and user interaction

### Terminal Processing
- **UART Reception**: Interrupt-driven character buffering
- **ANSI Processing**: Full VT100 escape sequence interpretation
- **Character Rendering**: Font-based text output with color support
- **Keyboard Input**: PS/2 and USB key processing with layout mapping
- **Screen Management**: Scrolling, clearing, cursor control

### Graphics Pipeline
- **Framebuffer Management**: Direct pixel manipulation with DMA acceleration
- **Font Rendering**: Bitmap-based character drawing with multiple modes
- **Color System**: 8-bit indexed color with configurable palettes
- **Screen Operations**: Hardware-accelerated scrolling and clearing

## Troubleshooting

### Common Issues
- **No Display**: Check HDMI connection and try different resolution
- **No UART**: Verify baud rate (115200) and wiring
- **Keyboard Not Working**: Check PS/2 connections or try USB keyboard
- **Boot Fails**: Ensure correct kernel*.img for your Pi model

### Debug Information
- **Boot Messages**: Enable debug logging for detailed startup information
- **Configuration**: Use setup dialog to verify and adjust settings
- **Hardware**: Check LED heartbeat to confirm system is running

## License

Copyright (C) 2014-2020 Filippo Bergamasco, Christian Lehner  
Copyright (C) 2025 Ralf Zühlsdorff (Enhanced Edition)

This project is licensed under the MIT License - see the LICENSE file for details.

## Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Make your changes with appropriate documentation
4. Test on multiple Pi models if possible
5. Submit a pull request

## Acknowledgments

- **Original Authors**: Filippo Bergamasco, Christian Lehner
- **USB Support**: USPI library integration
- **Community**: Contributors and testers
- **Documentation**: Enhanced Edition improvements and unified build system
