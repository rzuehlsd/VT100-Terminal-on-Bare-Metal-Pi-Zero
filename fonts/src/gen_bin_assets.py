import os
import re

BIN_DIR = "bin"
OUTFILE = "../src/binary_assets.s"  # Adjust path as needed

def font_symbol_name(bin_filename):
    # Example: spleen-6x12.bin -> G_SPLEEN6X12_GLYPHS
    base = os.path.splitext(os.path.basename(bin_filename))[0]
    # Remove dashes and make uppercase
    symbol = "G_" + base.replace("-", "_").replace("x", "X").upper() + "_GLYPHS"
    return symbol

def extract_wh(bin_filename):
    # Extract width and height from filename, e.g. system-8x8.bin -> (8, 8)
    match = re.search(r'(\d+)x(\d+)', bin_filename)
    if match:
        return int(match.group(1)), int(match.group(2))
    return (9999, 9999)  # Put unrecognized at the end

def main():
    bin_files = [f for f in os.listdir(BIN_DIR) if f.endswith(".bin")]
    # Sort by width, then height
    bin_files.sort(key=lambda f: extract_wh(f))
    lines = []
    lines.append(".section .rodata\n")
    for binfile in bin_files:
        symbol = font_symbol_name(binfile)
        lines.append(f".global {symbol}")
        lines.append(".align 4")
        lines.append(f"{symbol}: .incbin \"fonts/{BIN_DIR}/{binfile}\"\n")
    with open(OUTFILE, "w") as f:
        f.write("\n".join(lines))
    print(f"Wrote {OUTFILE} with {len(bin_files)} font entries.")

if __name__ == "__main__":
    main()