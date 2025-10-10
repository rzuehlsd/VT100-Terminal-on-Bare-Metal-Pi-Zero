# VT100 Terminal on Bare-Metal Pi (based on PiGFX)

This codebase is derived from PiGFX by Filippo Bergamasco.

Original project:

- PiGFX by Filippo Bergamasco (MIT License)
- Repository: [https://github.com/fbergama/pigfx](https://github.com/fbergama/pigfx)

We keep the original MIT license; see `LICENSE` in this repository for details.

## What’s different in this fork

I changed the original code to create a replica of the DEC VT100 terminal, both in software and hardware. I used a 60% size 3D model of the VT100 terminal, which was reated by Megardi (https://www.instructables.com/23-Scale-VT100-Terminal-Reproduction/) for printing the case. Be aware that the printed parts of the 3D model, at least in my experience, not allways fit very well and that you need to do some filling and a lot of sanding to get the case in shape.

The PiGFX implementation was a very good start for the VT100, but to implement my additional requirements I had to add some changes and on the way also fixed some glitches.

I wanted to replicate the real feeling of a VT100 terminal including fonts and bell. I did not intend to create a 100% emulation of a real VT100. I just wanted a bare metal implementation that runs on a Pi zero, starts up within seconds and gives me a "retro" feeling when playing with my vintage computer stuff. 

If you feel you need a close replication of the original VT100 please refer to the work of Lars Brinkhoff (https://github.com/larsbrinkhoff/terminal-simulator). 


## Hardware

Main reason for my additional software requirements was, that I also created an adapterboard for a Pi Zero (any other Pi should also work). The board can be used to power the Pi and an 8'' TFT Display (I used one that only needs 5VDC) from a 7.5 to 9V DC or AC plug-in power supply. The board also provides a DIN6 connector to directly connect to my MBC2-Z80 board, a RS232 port and a USB Type A connector to interface to standard USB keyboards. I also implemented a relais to switch the TxD and RxD lines of the Pi zero as I discovered that real nullmodem kabels are not easy to find. I added a simple buzzer to the board to simulate the 785Hz bell tone of the VT100 via software PWM.

The KiCAD files are provided in the repository in directory hardware. for detailed description see [Hardware README.md](hardware/README.md)

## List of Modifications

The following modifications and enhancements have been implemented:

- Reorganization of Font Build System (see [Font system details](FONT_SYSTEM.md)
- On screen setup dialog and file based configuration (see [Configuration and Setup](CONFIGURATION.md))
- Enhancements:
  - Added a “Switch Rx<>Tx” toggle in the setup dialog and applied the switch immediately on save
  - Polished auto-repeat handling; repeat delay and rate are configurable in the setup dialog
  - Generate bell sound via software PWM with configurable sound level


## Upstream compatibility

This fork tracks the upstream repository via a separate `upstream` remote. Where sensible, changes are kept minimal and localized to preserve compatibility with ongoing upstream development.

## License

MIT License © Original authors and contributors.
See `LICENSE` for details.
