# UART Helper Tool

A small Python script to send PiGFX graphics commands over a serial port.

## Install

```bash
python3 -m pip install -r requirements.txt
```

## Usage

```text
python3 uart_send.py PORT BAUD <command> [args]
```

Commands:

- load-ascii index width height base pixels [--rle]
  - pixels: comma-separated list; with --rle use value,count pairs
- load-bin index width height file
- blit index x y
- palette-select idx
- palette-upload base values
  - values: comma-separated 0xRRGGBB in given base (10 or 16)

Examples:

```bash
# Binary bitmap load + blit
python3 uart_send.py /dev/ttyUSB0 115200 load-bin 0 32 32 sample.raw
python3 uart_send.py /dev/ttyUSB0 115200 blit 0 10 10

# Upload three colors (hex) and switch to VGA palette
python3 uart_send.py /dev/ttyUSB0 115200 palette-upload 16 FF0000,00FF00,0000FF
python3 uart_send.py /dev/ttyUSB0 115200 palette-select 1
```
