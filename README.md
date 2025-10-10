# VT100 Terminal on Bare-Metal Pi (based on PiGFX)

This codebase is derived from PiGFX by Filippo Bergamasco.

Original project:

- PiGFX by Filippo Bergamasco (MIT License)
- Repository: [https://github.com/fbergama/pigfx](https://github.com/fbergama/pigfx)

We keep the original MIT license; see `LICENSE` in this repository for details.

## What’s different in this fork

I changed the original code to create a replica of the DEC VT100 terminal, both in software and hardware. I used a 60% size 3D model of the VT100 terminal, which was created by Megardi (https://www.instructables.com/23-Scale-VT100-Terminal-Reproduction/) for printing the case. 

The PiGFX implementation was a very good start for the VT100, but to implement my additional requirements I had to add some changes and on the way also fixed some glitches.

I wanted to replicate the real feeling of a VT100 terminal including fonts and bell. I did not intend to create a 100% emulation of a real VT100. I just wanted a bare metal implementation that runs on a Pi zero, starts up within seconds and gives me a "retro" feeling when playing with my vintage computer stuff. 

Below you see the output of my MBC2-Z80 using CP/M 3.0 with DEC font VT100. If you look very close you can see the simulated scan lines. To me that is "vintage" enought.

<p align="center">
<img src="images/screen.jpg" alt="Screen" width=%">  
</p>

But if you feel you need a close replication of the original VT100 please refer to the work of Lars Brinkhoff (https://github.com/larsbrinkhoff/terminal-simulator). 


## Hardware

Main reason for my additional software requirements was, that I created an adapter board for a Pi Zero (any other Pi should also work). The board can be used to power the Pi and an 8'' TFT Display (I use one that only needs 5VDC) from a 7.5 to 9V DC or AC plug-in power supply. The board also provides a DIN6 connector to directly connect to my MBC2-Z80 board, a RS232 port and a USB Type A connector to interface to standard USB keyboards. 

I used a RS232 board that not only holds the DB9 connector but also a RS2232 chip. So I only needed to connect the 4 pins on the back to my board.

The cable you see going from the Pi to the board connects D+/D- with test points on the second usb connector on the Pi zero (see https://maker-tutorials.com/raspberry-pi-zero-mit-usb-buchse-typ-a-erweitern-anloeten/).

I also implemented a relais to switch the TxD and RxD lines of the Pi zero as I discovered that real nullmodem kabels are not easy to find. Finally I added a simple buzzer to the board to simulate the 785Hz bell tone of the VT100 via software PWM. The following picture shows the prototype of the board (without buzzer).

<p align="center">
<img src="images/board.jpg" alt="Prototype" width="50%">  
</p>

The KiCAD files are provided in the repository in directory hardware. for detailed description see [Hardware README.md](hardware/README.md)

## List of Modifications

The following modifications and enhancements have been implemented:

- Reorganization of Font Build System (see [Font system details](FONT_SYSTEM.md)
- On screen setup dialog and file based configuration (see [Configuration and Setup](CONFIGURATION.md))
- Rearanging the build system and Makefile
- Implemented Enhancements:
  - Dynamic switching between screen resolutions 640x480, 800x640 and 1024x768
  - Added a “Switch Rx<>Tx” toggle in the setup dialog and applied the switch immediately on save to switch Rx anf Tx through relais
  - Polished auto-repeat handling; repeat delay and rate are configurable in the setup dialog
  - Generate bell sound via software PWM with configurable sound level using a simple passive buzzer


## Enhanced Build System

### Unified Build System (2025)

The font build system is described in link above. The main make file has been modified to use a varibale to control the build for different targets and also rebuilds the USB library:

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

```
Creating kernel.img for Raspberry Pi 1
==========================================
Build completed for Raspberry Pi 1
Target: kernel
Toolchain: arm-none-eabi-
USPI: Included for Pi 1
==========================================
```


## Remarks on building the VT100 Case

The VT100 case was printed on a Bamboo Lab P1S 3d-printer using the stl files provided by Megardi. Printing took some time (about 15 hours in total). When I tried to glue the pieces together I discovered that the printed parts of the 3D model, at least in my experience, not allways fit very well together. It took at lot of  filling and sanding to get the case in shape.

I than applied a filler in light grey to smooth the surface and prepare the case for painting. I painted the case in color RAL1015 "oyster white" which to my opinion comes very cloth to the original color.

<p align="center">
<img src="images/Case_1.jpg" alt="Case initial" width="30%">  <img src="images/Case_2.jpg" alt="Case final" width="55%"> 
</p>


## Upstream compatibility

This fork tracks the upstream repository via a separate `upstream` remote. Where sensible, changes are kept minimal and localized to preserve compatibility with ongoing upstream development.

## License

MIT License © Original authors and contributors.
See `LICENSE` for details.
