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
    if len(sys.argv) != 4:
        print("Usage: python3 switch_glyphs_in_font_png.py <fontfile-WxH.png> <glyph_index1> <glyph_index2>")
        sys.exit(1)
    pngfile = sys.argv[1]
    idx1 = int(sys.argv[2])
    idx2 = int(sys.argv[3])

    w, h = extract_wh(pngfile)
    img = Image.open(pngfile)
    img_w, img_h = img.size

    glyphs_per_row = img_w // w
    glyphs_per_col = img_h // h
    total_glyphs = glyphs_per_row * glyphs_per_col

    if not (0 <= idx1 < total_glyphs and 0 <= idx2 < total_glyphs):
        print(f"Glyph indices must be between 0 and {total_glyphs-1}")
        sys.exit(1)

    # Calculate positions
    gx1, gy1 = idx1 % glyphs_per_row, idx1 // glyphs_per_row
    gx2, gy2 = idx2 % glyphs_per_row, idx2 // glyphs_per_row

    box1 = (gx1 * w, gy1 * h, (gx1 + 1) * w, (gy1 + 1) * h)
    box2 = (gx2 * w, gy2 * h, (gx2 + 1) * w, (gy2 + 1) * h)

    glyph1 = img.crop(box1)
    glyph2 = img.crop(box2)

    img.paste(glyph2, box1)
    img.paste(glyph1, box2)

    outname = os.path.splitext(pngfile)[0] + f"_swapped_{idx1}_{idx2}.png"
    img.save(outname)
    print(f"Swapped glyph {idx1} and {idx2}. Saved as {outname}")

if __name__ == "__main__":
    main()