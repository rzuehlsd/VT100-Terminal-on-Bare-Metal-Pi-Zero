import os
import re
import subprocess
import sys

def extract_wh(filename):
    """
    Extracts width and height from a filename like NAME-WxH.png
    Returns (width, height) as integers, or (None, None) if not found.
    """
    basename = os.path.basename(filename)
    match = re.search(r'-([0-9]+)x([0-9]+)\.png$', basename, re.IGNORECASE)
    if match:
        return int(match.group(1)), int(match.group(2))
    return None, None

def main():
    import argparse
    parser = argparse.ArgumentParser(description="Frontend for buildfont: auto-extracts width/height from font PNG filename.")
    parser.add_argument("pngfile", help="Input PNG font file (e.g. VT100-10x20.png)")
    parser.add_argument("-o", "--output", help="Output BIN file (default: same name with .bin extension)")
    parser.add_argument("-c", "--color", default="1", help="Color mode for buildfont (default: 1)")
    parser.add_argument("-q", "--quiet", action="store_true", help="Quiet mode for buildfont")
    parser.add_argument("--buildfont", default="./buildfont", help="Path to buildfont executable")
    args = parser.parse_args()

    width, height = extract_wh(args.pngfile)
    if width is None or height is None:
        print("Error: Could not extract width and height from filename:", args.pngfile)
        print("Expected pattern: <name>-<width>x<height>.png (e.g. VT100-10x20.png)")
        sys.exit(1)

    output = args.output
    if not output:
        output = os.path.splitext(args.pngfile)[0] + ".bin"

    cmd = [
        args.buildfont,
        "-i", args.pngfile,
        "-o", output,
        "-w", str(width),
        "-h", str(height),
        "-c", str(args.color)
    ]
    if args.quiet:
        cmd.append("-q")

    print("Running:", " ".join(cmd))
    result = subprocess.run(cmd)
    sys.exit(result.returncode)

if __name__ == "__main__":
    main()