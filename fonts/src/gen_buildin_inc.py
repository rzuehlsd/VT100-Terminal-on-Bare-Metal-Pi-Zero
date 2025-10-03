import os

BIN_DIR = "bin"
OUTFILE = "../src/buildin_fonts.inc"

def font_symbol_name(bin_filename):
    # Example: spleen-6x12.bin -> G_SPLEEN6X12_GLYPHS
    base = os.path.splitext(os.path.basename(bin_filename))[0]
    symbol = "G_" + base.replace("-", "_").replace("x", "X").upper() + "_GLYPHS"
    return symbol

def font_display_name(bin_filename):
    # Example: spleen-6x12.bin -> Spleen 6x12
    base = os.path.splitext(os.path.basename(bin_filename))[0]
    # Replace dashes with spaces, capitalize, and keep x lowercase for size
    parts = base.split('-')
    if len(parts) > 1:
        name = parts[0].capitalize() + " " + parts[1].lower()
    else:
        name = base.capitalize()
    return name

def font_size(bin_filename):
    # Example: spleen-6x12.bin -> (6, 12)
    base = os.path.splitext(os.path.basename(bin_filename))[0]
    import re
    match = re.search(r'(\d+)x(\d+)', base)
    if match:
        return int(match.group(1)), int(match.group(2))
    return (8, 16)  # Default fallback

def main():
    bin_files = sorted(f for f in os.listdir(BIN_DIR) if f.endswith(".bin"))
    lines = []
    lines.append("// This file is generated automatically. Do not edit!")
    lines.append("// It contains the registration code for all built-in fonts compiled into PiGFX.\n")
    lines.append("// Externs from binary_assets.s")
    for binfile in bin_files:
        symbol = font_symbol_name(binfile)
        lines.append(f"extern unsigned char {symbol}[];")
    lines.append("\n\n")
    lines.append("void gfx_register_builtin_fonts(void)")
    lines.append("{")
    lines.append("    // Register all built-in fonts")
    lines.append("    // 8x16 System Font is the system default font (index 0)\n")
    for i, binfile in enumerate(bin_files):
        symbol = font_symbol_name(binfile)
        name = font_display_name(binfile)
        w, h = font_size(binfile)
        # The first font is always called "System 8x16" for compatibility
        if i == 0:
            reg_name = "System 8x16"
        else:
            reg_name = name
        lines.append(f'    font_registry_register("{reg_name}", {w}, {h}, {symbol}, font_get_glyph_address);')
    lines.append("}")
    with open(OUTFILE, "w") as f:
        f.write("\n".join(lines) + "\n")
    print(f"Wrote {OUTFILE} with {len(bin_files)} font entries.")

if __name__ == "__main__":
    main()