# Bare Metal VT100 Terminal auf Pi Zero 

## Test ohne neu compilieren

### Setup

- Pi Zero mit USB Keyboard über USB Hub mit externen Netzteil
- Auf SD Karte ist der aktuelle Inhalt des Verzeichnisses bin kopiert mit bootcode.bin und start.elf aus bin Verzeichnis von dhansel
- Folgende Änderungen am pigfX:
```
[UART] 
baudrate = 115200             ; Baudrate for the UART interface, should work fine between 300 and 115200 baud

[Input]
useUsbKeyboard = 1          ; With this disabled, PiGFX will not load the uspi and look for a connected keyboard on any USB port
keyboardLayout = de         ; Keyboard layout to be used. These are supported: us, uk, it, fr, es, de, sg
sendCRLF = 1                ; With this enabled, the enter key on the connected keyboard will generate CRLF instead of LF only. CR has value 0x0D, LF has value 0x0A.
```
- Pi wurde durch Verbinden von GPIO15 und GPIO16 auf Loopback konfiguriert

### Ergebnis
- Pi booted und USB Keyboard wird erkannt mit Layout DE
- SD Image wird gesichert im Verzeichnis images

### Build

**Important: Configuration for Pi Zero (RASPPI=1)**

All makefiles must be configured for Pi Zero Gen 1 to avoid "undefined instruction error" crashes:

- **Main Makefile**: `RPI ?= 1` (already correctly set - defaults to Pi Zero/Pi 1)
- **uspi/Config.mk**: `RASPPI = 1` (must be explicitly set for Pi Zero)
- **uspi/Rules.mk**: `RASPPI ?= 1` (already correctly set - defaults to Pi Zero)

The architectural mismatch (building for Pi 3/4 instead of Pi Zero) was the main cause of boot crashes.

**Build Process:**

- uspi sources clonen
```
git clone https://github.com/rsta2/uspi.git
````
- **Ensure uspi/Config.mk is set correctly:**
```
echo "RASPPI = 1" > uspi/Config.mk
```
- Im Verzeichnis ./uspi/lib diefolgenden Befehle ausführen:
````
make clean
make all
````
Im Verzeichnis lib wird danach neben den Object Files auch die Library libuspi.a erzeugt.

- Danach in das pigfx Verzeichnis wechseln und eine neue Version mit
````
make clean
make
````
bauen und den Inhalt des kompletten bin Verzeichnisses auf die SD Karte kopieren. 

- Pi startet mit neuem pigfx kernel und den aktuellen Konfigurationen im File pigfx.txt

**Troubleshooting:**
- If kernel crashes with "undefined instruction error": Check that all RASPPI/RPI settings are set to 1
- Kernel size should be around 314-315KB for correct Pi Zero build
- C99 syntax (like `for (int i = 0; ...)`) must be avoided - use C89/C90 standard


