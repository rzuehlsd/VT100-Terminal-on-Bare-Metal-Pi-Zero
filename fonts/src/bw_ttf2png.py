# bw_ttf2png.py

import argparse
from PIL import Image, ImageDraw, ImageFont
from collections import Counter

parser = argparse.ArgumentParser(description="Render ASCII table to PNG.")
parser.add_argument('--cell-width', type=int, default=20, help='Width of each cell in pixels')
parser.add_argument('--cell-height', type=int, default=40, help='Height of each cell in pixels')
parser.add_argument('--font-size', type=int, default=40, help='Font size')
parser.add_argument('--font-path', type=str, default="/Users/rz/Library/Fonts/Glass_TTY_VT220.ttf", help='Path to TTF font')
parser.add_argument('--output', type=str, default="ascii_table.png", help='Output PNG filename')
parser.add_argument('--threshold', type=int, default=128, help='Threshold for black/white conversion (0-255)')
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
threshold = args.threshold

# Calculate rows needed
num_chars = end_code - start_code + 1
rows = (num_chars + cols - 1) // cols

# Create image in grayscale for better text rendering
img_width = cols * cell_width
img_height = rows * cell_height
image = Image.new("L", (img_width, img_height), 0)  # Grayscale, black background
draw = ImageDraw.Draw(image)

# Load font
try:
    font = ImageFont.truetype(font_path, font_size)
except OSError:
    font = ImageFont.load_default()
    print("Default font loaded")

# Draw characters in white
for i, code in enumerate(range(start_code, end_code + 1)):
    row = i // cols
    col = i % cols
    x = col * cell_width
    y = row * cell_height
    ch = chr(code)
    draw.text((x, y), ch, font=font, fill=255)  # White text

# Analyze grayscale image before conversion
print("=== Grayscale Image Analysis ===")
grayscale_pixels = list(image.getdata())
grayscale_counter = Counter(grayscale_pixels)
total_pixels = len(grayscale_pixels)

print(f"Total pixels: {total_pixels}")

# Create batched histogram
def create_batched_histogram(counter, batch_size=8):
    # Create batches
    batches = {}
    for value, count in counter.items():
        batch_start = (value // batch_size) * batch_size
        batch_end = batch_start + batch_size - 1
        batch_key = f"{batch_start:3d}-{batch_end:3d}"
        if batch_key not in batches:
            batches[batch_key] = 0
        batches[batch_key] += count
    
    # Find max count for scaling
    max_count = max(batches.values()) if batches else 0
    scale_factor = 50 / max_count if max_count > 0 else 1
    
    print("\nPixel value distribution (batched):")
    print("Range     Count      Percentage  Histogram")
    print("-" * 60)
    
    for batch_range in sorted(batches.keys()):
        count = batches[batch_range]
        percentage = (count / total_pixels) * 100
        bar_length = int(count * scale_factor)
        bar = "█" * bar_length
        print(f"{batch_range}: {count:8d} ({percentage:5.1f}%) {bar}")

create_batched_histogram(grayscale_counter)

# Convert to true black and white using threshold
def apply_threshold(pixel):
    return 255 if pixel > threshold else 0

# Apply threshold and convert to 1-bit mode for true black/white
image = image.point(apply_threshold, mode='1')

# Analyze black and white image after conversion
print(f"\n=== Black/White Image Analysis (threshold: {threshold}) ===")
bw_pixels = list(image.getdata())
bw_counter = Counter(bw_pixels)

print("Pixel value distribution after threshold:")
for value in sorted(bw_counter.keys()):
    count = bw_counter[value]
    percentage = (count / total_pixels) * 100
    color_name = "Black" if value == 0 else "White"
    bar_length = int((count / total_pixels) * 50)
    bar = "█" * bar_length
    print(f"  {value:3d} ({color_name:5s}): {count:8d} ({percentage:5.1f}%) {bar}")

# Save image
image.save(output_file)
print(f"\nSaved true black and white PNG to {output_file}")