# Carrier Board for Pi Zero VT100 Terminal
<div align="center">
RZ 9/2015
</br>
</br>
</div>


This project designs and builds a carrier board for the Raspberry Pi Zero that, together with the pigfx_V20 software, enables a bare‑metal implementation of a VT100 terminal.

The software features are described in the directory PIGFX_20, including the enhancements over the original version. This software version extends the PiGFX project by Filippo Bergamasco.

The carrier board can be mounted directly in the rear panel of a terminal enclosure and provides the following components:
- 5 VDC / 2 A power supply (input 7–9 V DC or AC / 2 A wall adapter)
- RS‑232 interface to the host computer
- Alternative host interface via Mini‑DIN‑6 connector and 4‑pin header
- USB‑A connector for a wired USB keyboard
- Switch (relay) to swap RxD and TxD
- 800 Hz buzzer (optional)

The Pi Zero GPIO pins are not 5 V tolerant. Therefore, all incoming signals (here: RxD only) must be level‑shifted from 5 V to 3.3 V. Outgoing signals are 5 V‑level compatible for typical systems.

The RxD level shifting is implemented with a resistor divider: 820 Ω and 1.5 kΩ. No issues have been observed even at 115200 baud.

The updated version was prototyped on the old PCB. The 74HC5040 was desoldered and the RxD signal was routed to the Pi through the resistor divider. All other signals were wired directly to the Pi Zero with jumpers.

Testing also showed that the 7805 linear regulator needs a small heatsink for stable operation. On the old board, a small heatsink was provisionally installed for testing. With the heatsink, the 7805 ran stably and supplied power not only to the Pi Zero but also to the MSBC2‑Z80 running CP/M. 

In the final version, the LCD display must also be supplied from the 5 V rail with about 1 A. Therefore, the 5 V supply must provide at least 2 A and use a larger heatsink with about 6 K/W thermal resistance. Therefore the 7805 was replaced by a LM350 regulator, which can provided up to 3A with proper heatsink.

All changes have been reflected in the schematic and layout.

An adapter for the MBC2‑Z80 to connect the Z80 via the Mini‑DIN‑8 connector is described in the sub‑project “Z80‑SBC_Adapter”.

Board revisions:
- [x] Barrel jack for 9 V DC / AC input
- [x] Replace regulator with LM350
- [x] Check heatsink and install a larger one if needed
- [x] LED indicator On/Off
- [x] Contacts for power switch
- [x] RxD <> TxD swap via relay, switchable by Pi GPIO pin
- [x] USB‑A socket for keyboard connection
- [x] Mini‑DIN‑6 socket for direct MBC2 connection with power
- [x] RS‑232 DB9 connector
- [x] Internal header for direct MBC2 connection inside the terminal enclosure
- [x] PCB cut‑out for the USB plug (very short pins)
- [x] Buzzer / speaker circuit added
    - [x] Pads for piezo buzzer, 12.5 mm diameter, 5 mm pitch
- [x] Schematic revised for external fabrication