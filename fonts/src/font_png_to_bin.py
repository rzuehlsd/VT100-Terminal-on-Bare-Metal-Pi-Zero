# font_png_to_bin.py
import sys
import os
import re
from PIL import Image

def extract_wh(filename):
    """Extract width and height from filename like NAME-WxH.png"""
    match = re.search(r'-([0-9]+)x([0-9]+)\.png$', filename, re.IGNORECASE)
    if match:
        return int(match.group(1)), int(match.group(2))
    raise ValueError("Filename must contain -<width>x<height>.png")

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 font_png_to_bin.py <fontfile-WxH.png> [output.bin]")
        sys.exit(1)
    pngfile = sys.argv[1]
    outbin = sys.argv[2] if len(sys.argv) > 2 else os.path.splitext(pngfile)[0] + ".bin"
    w, h = extract_wh(pngfile)

    img = Image.open(pngfile).convert("L")  # Convert to grayscale
    img_w, img_h = img.size

    # Calculate number of glyphs per row and column
    glyphs_per_row = img_w // w
    glyphs_per_col = img_h // h
    total_glyphs = glyphs_per_row * glyphs_per_col

    print(f"Image size: {img_w}x{img_h}, Glyph size: {w}x{h}, Total glyphs: {total_glyphs}")

    # Prepare output buffer
    out_bytes = bytearray()

    for glyph_idx in range(total_glyphs):
        gx = glyph_idx % glyphs_per_row
        gy = glyph_idx // glyphs_per_row
        left = gx * w
        upper = gy * h
        glyph = img.crop((left, upper, left + w, upper + h))
        for y in range(h):
            for x in range(w):
                pixel = glyph.getpixel((x, y))
                out_bytes.append(255 if pixel < 128 else 0)  # 255 for set, 0 for background

    with open(outbin, "wb") as f:
        f.write(out_bytes)
    print(f"Wrote {outbin} with {total_glyphs} glyphs of size {w}x{h}")

if __name__ == "__main__":
    main()