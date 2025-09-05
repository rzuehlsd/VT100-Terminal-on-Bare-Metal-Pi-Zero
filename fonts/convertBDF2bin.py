# SPDX-FileCopyrightText: 2021 ladyada for Adafruit Industries
# SPDX-License-Identifier: MIT

"""
This example loads a font and uses it to print an
ASCII art representation of the given string specimen
"""

from adafruit_bitmap_font import bitmap_font

font_file = "./VT220_Fonts/VT220-32x64.bdf"
output_file = './VT220-32x64.bin'

out = open("output.txt", "a")

font = bitmap_font.load_font(font_file)
width, height, dx, dy = font.get_bounding_box()
print("font width, height, dx, dy:", width, height, dx, dy, file=out)

with open(output_file, 'wb') as f:
    # Process all 256 glyphs
    for code in range(256):
        font.load_glyphs(chr(code))
        glyph = font.get_glyph(code)
        print("Processing code: ", code, " char = ", chr(code), file=out)
        print("Processing char: ", chr(code), " Code = ", code) 
        cell = [[0 for _ in range(width)] for _ in range(height)]
        lines = [["-" for _ in range(width)] for _ in range(height)]
        if glyph:
            print("glyph width, height, dx, dy:", glyph.width, glyph.height, glyph.dx, glyph.dy, file=out)
            yoffset = (height + dy) - (glyph.height + glyph.dy)
            xoffset = glyph.dx + dx
            print("y offset:", yoffset, " x offset:", xoffset, file=out)

            for y in range(glyph.height):
                for x in range(glyph.width):
                    value = glyph.bitmap[x, y]
                    cell_y = y + yoffset
                    cell_x = x + xoffset
                    
                    if 0 <= cell_x < width and 0 <= cell_y < height:
                        cell[cell_y][cell_x] = 255 if value > 0 else 0
                        lines[cell_y][cell_x] = "#" if value > 0 else "."
        # Write the cell row by row
        
        for row in cell:
            f.write(bytes(row))
        for line in lines:
            print("".join(line), file=out)

    f.close()
out.close()