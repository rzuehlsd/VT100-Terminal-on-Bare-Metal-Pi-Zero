# png_analyzer.py

import argparse
from PIL import Image
from collections import Counter

def create_batched_histogram(counter, total_pixels, batch_size=8):
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

def analyze_image_detailed(counter, total_pixels):
    print("\nDetailed pixel value distribution:")
    print("Value  Count      Percentage")
    print("-" * 30)
    
    for value in sorted(counter.keys()):
        count = counter[value]
        percentage = (count / total_pixels) * 100
        if percentage > 0.1:  # Only show values with >0.1% occurrence
            print(f"{value:3d}: {count:8d} ({percentage:5.1f}%)")

def main():
    parser = argparse.ArgumentParser(description="Analyze grayscale PNG file pixel distribution.")
    parser.add_argument('input', help='Input PNG file to analyze')
    parser.add_argument('--batch-size', type=int, default=8, help='Batch size for histogram (default: 8)')
    parser.add_argument('--detailed', action='store_true', help='Show detailed per-value distribution')
    args = parser.parse_args()

    try:
        # Load image and convert to grayscale if needed
        image = Image.open(args.input)
        if image.mode != 'L':
            print(f"Converting from {image.mode} to grayscale...")
            image = image.convert('L')
        
        print(f"=== Analyzing: {args.input} ===")
        print(f"Image size: {image.width} x {image.height}")
        print(f"Image mode: {image.mode}")
        
        # Get pixel data
        pixels = list(image.getdata())
        counter = Counter(pixels)
        total_pixels = len(pixels)
        
        print(f"Total pixels: {total_pixels}")
        
        # Show basic statistics
        min_val = min(counter.keys())
        max_val = max(counter.keys())
        unique_values = len(counter.keys())
        
        print(f"Pixel value range: {min_val} - {max_val}")
        print(f"Unique pixel values: {unique_values}")
        
        # Show most common values
        print(f"\nTop 5 most common pixel values:")
        for value, count in counter.most_common(5):
            percentage = (count / total_pixels) * 100
            print(f"  Value {value:3d}: {count:8d} pixels ({percentage:5.1f}%)")
        
        # Show batched histogram
        create_batched_histogram(counter, total_pixels, args.batch_size)
        
        # Show detailed distribution if requested
        if args.detailed:
            analyze_image_detailed(counter, total_pixels)
        
        # Suggest threshold values
        print(f"\n=== Threshold Suggestions ===")
        # Find valley between black and white peaks
        black_peak = 0
        white_peak = 255
        
        # Simple threshold suggestions
        otsu_threshold = 128  # Simplified - could implement real Otsu
        mean_threshold = sum(val * count for val, count in counter.items()) // total_pixels
        
        print(f"Mean-based threshold: {mean_threshold}")
        print(f"Mid-range threshold: {(min_val + max_val) // 2}")
        print(f"Standard threshold: 128")
        
        # Check if image is already mostly black/white
        black_pixels = counter.get(0, 0)
        white_pixels = counter.get(255, 0)
        bw_percentage = ((black_pixels + white_pixels) / total_pixels) * 100
        
        if bw_percentage > 90:
            print(f"\nNote: Image is already {bw_percentage:.1f}% black/white")
        else:
            print(f"\nNote: Image has {100 - bw_percentage:.1f}% grayscale values")
            
    except FileNotFoundError:
        print(f"Error: File '{args.input}' not found")
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    main()