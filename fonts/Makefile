PNG_DIR := png
BIN_DIR := bin

# Find all PNG files in png directory
PNGS := $(wildcard $(PNG_DIR)/*.png)
BINS := $(patsubst $(PNG_DIR)/%.png, $(BIN_DIR)/%.bin, $(PNGS))

all: buildfont $(BINS) ../src/binary_assets.s ../src/buildin_fonts.inc

buildfont: src/buildfont.cpp src/CImg.h
	g++ src/buildfont.cpp -o buildfont

# General rule: convert each PNG in png/ to BIN in bin/
$(BIN_DIR)/%.bin: $(PNG_DIR)/%.png buildfont
	@mkdir -p $(BIN_DIR)
	python3 buildfont.py $< -o $@ -c 1 -q

../src/binary_assets.s: $(BINS)
	python3 src/gen_bin_assets.py $(BINS) > ./src/binary_assets.s

../src/buildin_fonts.inc: $(BINS)
	python3 src/gen_buildin_inc.py $(BINS) > ./src/buildin_fonts.inc

clean:
	rm -f buildfont
	rm -f $(BIN_DIR)/*.bin
	rm -f ./src/buildin_fonts.inc
	rm -f ./src/binary_assets.s
