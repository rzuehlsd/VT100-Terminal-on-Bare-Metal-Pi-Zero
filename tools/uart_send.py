#!/usr/bin/env python3
"""
PiGFX UART sender tool

Send graphics commands to a PiGFX-based terminal over a serial port.
- Load bitmaps (ASCII/binary, optional RLE)
- Blit loaded bitmaps
- Upload custom palettes

Requirements: pyserial
"""
from __future__ import annotations
import argparse
import serial
import sys
from typing import Iterable, Sequence

ESC = "\x1b"

def write(port: serial.Serial, data: bytes) -> None:
    port.write(data)
    port.flush()


def esc(seq: str) -> bytes:
    return (ESC + seq).encode("ascii")


def send_ascii_values(port: serial.Serial, values: Iterable[int], base: int) -> None:
    sep = b";"
    if base == 10:
        for v in values:
            write(port, str(int(v)).encode("ascii"))
            write(port, sep)
    elif base == 16:
        for v in values:
            write(port, f"{int(v):02X}".encode("ascii"))
            write(port, sep)
    else:
        raise ValueError("base must be 10 or 16")


def cmd_load_bitmap_ascii(port: serial.Serial, index: int, width: int, height: int, base: int, pixels: Sequence[int], rle: bool=False) -> None:
    final = 'A' if rle else 'a'
    write(port, esc(f"[{index};{width};{height};{base}{final}"))
    if rle:
        # Expect pixels as (value, count) pairs
        assert len(pixels) % 2 == 0, "RLE pixels must be pairs: value,count"
    send_ascii_values(port, pixels, base)


def cmd_load_bitmap_binary(port: serial.Serial, index: int, width: int, height: int, data: bytes, rle: bool=False) -> None:
    final = 'B' if rle else 'b'
    write(port, esc(f"[{index};{width};{height}{final}"))
    write(port, data)


def cmd_blit_bitmap(port: serial.Serial, index: int, x: int, y: int) -> None:
    write(port, esc(f"[{index};{x};{y}d"))


def cmd_palette_select(port: serial.Serial, idx: int) -> None:
    write(port, esc(f"[={idx}p"))


def cmd_palette_upload_ascii(port: serial.Serial, base: int, colors: Iterable[int]) -> None:
    # Begin upload with base and count
    colors_list = list(colors)
    write(sys.stdout.buffer, b"Starting palette upload...\n")
    write(sys.stdout.buffer, b"\n")
    write(sys.stdout.buffer, b"Note: This prints to stdout if run without a connected device.\n")
    write(sys.stdout.buffer, b"\n")
    # The actual device command:
    # ESC[=base;countp then stream values
    # In case stdout is used for dry-run, this still prints the exact bytes


def cmd_palette_upload(port: serial.Serial, base: int, colors: Sequence[int]) -> None:
    write(port, esc(f"[={base};{len(colors)}p"))
    send_ascii_values(port, colors, base)


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Send PiGFX graphics commands over UART")
    p.add_argument("port", help="Serial port (e.g. /dev/ttyUSB0)")
    p.add_argument("baud", type=int, nargs="?", default=115200, help="Baud rate (default: 115200)")
    sub = p.add_subparsers(dest="cmd", required=True)

    # load-ascii
    la = sub.add_parser("load-ascii", help="Load bitmap via ASCII stream")
    la.add_argument("index", type=int)
    la.add_argument("width", type=int)
    la.add_argument("height", type=int)
    la.add_argument("base", type=int, choices=[10,16])
    la.add_argument("pixels", help="Comma-separated values; for RLE use value,count pairs")
    la.add_argument("--rle", action="store_true")

    # load-bin
    lb = sub.add_parser("load-bin", help="Load bitmap via binary bytes")
    lb.add_argument("index", type=int)
    lb.add_argument("width", type=int)
    lb.add_argument("height", type=int)
    lb.add_argument("file", help="Path to raw bytes file (width*height)")

    # blit
    bl = sub.add_parser("blit", help="Blit a loaded bitmap")
    bl.add_argument("index", type=int)
    bl.add_argument("x", type=int)
    bl.add_argument("y", type=int)

    # palette-select
    ps = sub.add_parser("palette-select", help="Select built-in palette index")
    ps.add_argument("idx", type=int, help="0..3 from src/palette.h")

    # palette-upload
    pu = sub.add_parser("palette-upload", help="Upload custom palette entries (ASCII)")
    pu.add_argument("base", type=int, choices=[10,16])
    pu.add_argument("values", help="Comma-separated RGB values (0xRRGGBB in given base)")

    return p.parse_args()


def main() -> int:
    ns = parse_args()

    try:
        with serial.Serial(ns.port, ns.baud, timeout=1) as ser:
            if ns.cmd == "load-ascii":
                vals = [int(x, 16 if ns.base == 16 and x.lower().startswith('0x') else ns.base) for x in ns.pixels.split(',')]
                cmd_load_bitmap_ascii(ser, ns.index, ns.width, ns.height, ns.base, vals, ns.rle)
            elif ns.cmd == "load-bin":
                data = open(ns.file, 'rb').read()
                cmd_load_bitmap_binary(ser, ns.index, ns.width, ns.height, data, rle=False)
            elif ns.cmd == "blit":
                cmd_blit_bitmap(ser, ns.index, ns.x, ns.y)
            elif ns.cmd == "palette-select":
                cmd_palette_select(ser, ns.idx)
            elif ns.cmd == "palette-upload":
                vals = [int(x, 16 if ns.base == 16 and x.lower().startswith('0x') else ns.base) for x in ns.values.split(',')]
                cmd_palette_upload(ser, ns.base, vals)
            else:
                print("Unknown command", file=sys.stderr)
                return 2
    except serial.SerialException as e:
        print(f"Serial error: {e}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
