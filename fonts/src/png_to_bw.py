# png_to_bw.py

import argparse
from PIL import Image
from collections import Counter

def main():
    parser = argparse.ArgumentParser(description="Convert grayscale PNG to black and white with threshold.")
    parser.add_argument('input', help='Input grayscale PNG file')
    parser.add_argument('output', help='Output black and white PNG file')
    parser.add_argument('--threshold', type=int, default=128, help='Threshold value (0-255, default: 128)')
    parser.add_argument('--invert', action='store_true', help='Invert the output (black becomes white, white becomes black)')
    parser.add_argument('--analyze', action='store_true', help='Show before/after pixel analysis')
    args = parser.parse_args()

    try:
        # Validate threshold
        if not 0 <= args.threshold <= 255:
            print("Error: Threshold must be between 0 and 255")
            return

        # Load image and convert to grayscale if needed
        image = Image.open(args.input)
        if image.mode != 'L':
            print(f"Converting from {image.mode} to grayscale...")
            image = image.convert('L')

        print(f"=== Converting: {args.input} -> {args.output} ===")
        print(f"Image size: {image.width} x {image.height}")
        print(f"Threshold: {args.threshold}")
        if args.invert:
            print("Inversion: ON")

        # Analyze original if requested
        if args.analyze:
            print("\n--- Before Conversion ---")
            pixels = list(image.getdata())
            counter = Counter(pixels)
            total_pixels = len(pixels)
            
            min_val = min(counter.keys())
            max_val = max(counter.keys())
            unique_values = len(counter.keys())
            
            print(f"Pixel value range: {min_val} - {max_val}")
            print(f"Unique pixel values: {unique_values}")
            
            # Show distribution around threshold
            below_threshold = sum(count for val, count in counter.items() if val < args.threshold)
            above_threshold = sum(count for val, count in counter.items() if val >= args.threshold)
            
            print(f"Pixels below threshold ({args.threshold}): {below_threshold} ({below_threshold/total_pixels*100:.1f}%)")
            print(f"Pixels above threshold ({args.threshold}): {above_threshold} ({above_threshold/total_pixels*100:.1f}%)")

        # Apply threshold conversion
        def apply_threshold(pixel):
            if args.invert:
                return 0 if pixel >= args.threshold else 255
            else:
                return 255 if pixel >= args.threshold else 0

        # Convert to black and white
        bw_image = image.point(apply_threshold, mode='1')

        # Analyze result if requested
        if args.analyze:
            print("\n--- After Conversion ---")
            bw_pixels = list(bw_image.getdata())
            bw_counter = Counter(bw_pixels)
            
            for value in sorted(bw_counter.keys()):
                count = bw_counter[value]
                percentage = (count / total_pixels) * 100
                color_name = "Black" if value == 0 else "White"
                print(f"  {color_name}: {count:8d} pixels ({percentage:5.1f}%)")

        # Save the result
        bw_image.save(args.output)
        print(f"\nSaved black and white PNG to: {args.output}")

        # Show file sizes
        import os
        original_size = os.path.getsize(args.input)
        new_size = os.path.getsize(args.output)
        compression_ratio = (1 - new_size / original_size) * 100
        
        print(f"Original file size: {original_size:,} bytes")
        print(f"New file size: {new_size:,} bytes")
        print(f"Size reduction: {compression_ratio:.1f}%")

    except FileNotFoundError:
        print(f"Error: File '{args.input}' not found")
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    main()