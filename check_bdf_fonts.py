from bdflib import reader
import glob
import os

print("CWD:", os.getcwd())
bdf_files = glob.glob("./fonts/VT220_Fonts/*.bdf")
print("Found files:", bdf_files)

for bdf_path in bdf_files:
    try:
        with open(bdf_path, "rb") as f:
            font = reader.read_bdf(f)
        print(f"{bdf_path}: VALID")
    except Exception as e:
        print(f"{bdf_path}: INVALID ({e})")