# print_ascii_to_png.py

import argparse
from PIL import Image, ImageDraw, ImageFont

parser = argparse.ArgumentParser(description="Render ASCII table to PNG.")
parser.add_argument('--cell-width', type=int, default=12, help='Width of each cell in pixels')
parser.add_argument('--cell-height', type=int, default=24, help='Height of each cell in pixels')
parser.add_argument('--font-size', type=int, default=24, help='Font size')
parser.add_argument('--font-path', type=str, default="/Users/rz/Library/Fonts/Glass_TTY_VT220.ttf", help='Path to TTF font')
parser.add_argument('--output', type=str, default="ascii_table.png", help='Output PNG filename')
args = parser.parse_args()

# Settings
start_code = 0
end_code = 255
cols = 16

cell_width = args.cell_width
cell_height = args.cell_height
font_size = args.font_size
font_path = args.font_path
output_file = args.output

# Calculate rows needed
num_chars = end_code - start_code + 1
rows = (num_chars + cols - 1) // cols

# Create image in 1-bit mode (black and white only)
img_width = cols * cell_width
img_height = rows * cell_height
image = Image.new("1", (img_width, img_height), 0)  # "1" = 1-bit pixels, 0 = black
draw = ImageDraw.Draw(image)

# Load font
try:
    font = ImageFont.truetype(font_path, font_size)
except OSError:
    font = ImageFont.load_default()
    print("Default font loaded")

# Draw characters
for i, code in enumerate(range(start_code, end_code + 1)):
    row = i // cols
    col = i % cols
    x = col * cell_width
    y = row * cell_height
    ch = chr(code)
    draw.text((x, y), ch, font=font, fill=1)  # fill=1 means white in 1-bit mode
    # Optionally, draw the code below the character:
    # draw.text((x, y + font_size), str(code), font=font, fill=1)

# Save image
image.save(output_file)
print("Saved to", output_file)
