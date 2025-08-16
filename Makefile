
RPI ?= 1

# Select appropriate toolchain based on RPI version
ifeq ($(strip $(RPI)),1)
ARMGNU ?= arm-none-eabi
CFLAGS = -Wall -Wextra -O2 -nostdlib -nostartfiles -fno-stack-limit -ffreestanding -fsigned-char -march=armv6j -mtune=arm1176jzf-s -mfloat-abi=soft -DRPI=1
else ifeq ($(strip $(RPI)),2)
ARMGNU ?= arm-none-eabi
CFLAGS = -Wall -Wextra -O2 -g -nostdlib -nostartfiles -fno-stack-limit -ffreestanding -fsigned-char -march=armv7-a -mtune=cortex-a7 -DRPI=2
else ifeq ($(strip $(RPI)),3)
ARMGNU ?= arm-none-eabi
CFLAGS = -Wall -Wextra -O2 -g -nostdlib -nostartfiles -fno-stack-limit -ffreestanding -fsigned-char -march=armv8-a -mtune=cortex-a53 -DRPI=3
else ifeq ($(strip $(RPI)),4)
ARMGNU ?= aarch64-linux-gnu
CFLAGS = -Wall -Wextra -O2 -g -nostdlib -nostartfiles -fno-stack-limit -ffreestanding -fsigned-char -march=armv8-a -mtune=cortex-a72 -DRPI=4
else
$(error Unsupported RPI version: $(RPI). Supported versions are 1, 2, 3, 4)
endif

# Display toolchain information
$(info Building for Raspberry Pi $(RPI) with toolchain: $(ARMGNU))

# Set PREFIX for uspi builds
PREFIX = $(ARMGNU)-

# Set target filename based on RPI version
ifeq ($(strip $(RPI)),1)
TARGET = kernel
else ifeq ($(strip $(RPI)),2)
TARGET = kernel7
else ifeq ($(strip $(RPI)),3)
TARGET = kernel8-32
else ifeq ($(strip $(RPI)),4)
TARGET = kernel8-64
endif

## Important!!! asm.o must be the first object to be linked!
OOB = asm.o exceptionstub.o synchronize.o mmu.o pigfx.o uart.o irq.o utils.o gpio.o mbox.o prop.o board.o actled.o framebuffer.o console.o gfx.o dma.o nmalloc.o uspios_wrapper.o ee_printf.o stupid_timer.o block.o emmc.o c_utils.o mbr.o fat.o config.o ini.o ps2.o keyboard.o setup.o font_registry.o binary_assets.o

BUILD_DIR = build
SRC_DIR = src
BUILD_VERSION = $(shell git describe --all --long | cut -d "-" -f 3)


OBJS=$(patsubst %.o,$(BUILD_DIR)/%.o,$(OOB))

LIBGCC=$(shell $(ARMGNU)-gcc -print-libgcc-file-name)

ifeq ($(strip $(RPI)),4)
LIBUSPI=
else
LIBUSPI=uspi/lib/libuspi.a
endif

all: uspi pigfx.elf pigfx.hex kernel 
	@echo "=========================================="
	@echo "Build completed for Raspberry Pi $(RPI)"
	@echo "Target: $(TARGET)"
	@echo "Toolchain: $(PREFIX)"
ifeq ($(strip $(RPI)),4)
	@echo "USPI: Not required for Pi 4"
else
	@echo "USPI: Included for Pi $(RPI)"
endif
	@echo "=========================================="
	ctags src/

# Configure and build uspi library for the specified RPI version
uspi: uspi/lib/libuspi.a

uspi/lib/libuspi.a: uspi/Config.mk
ifeq ($(strip $(RPI)),4)
	@echo "Skipping USPI build for Raspberry Pi 4"
else
	@echo "Building USPI for Raspberry Pi $(RPI) with toolchain $(PREFIX)"
	@$(MAKE) -C uspi/lib clean PREFIX=$(PREFIX)
	@$(MAKE) -C uspi/env/lib clean PREFIX=$(PREFIX)
	@$(MAKE) -C uspi/lib PREFIX=$(PREFIX)
	@$(MAKE) -C uspi/env/lib PREFIX=$(PREFIX)
endif

# Force regeneration of Config.mk when RPI version changes
uspi/Config.mk: FORCE
ifeq ($(strip $(RPI)),4)
	@echo "Skipping USPI config for Raspberry Pi 4"
else
	@echo "Configuring USPI for Raspberry Pi $(RPI)"
	@echo "RASPPI = $(RPI)" > uspi/Config.mk
endif

.PHONY: FORCE
FORCE:

$(SRC_DIR)/pigfx_config.h: pigfx_config.h.in 
	@sed 's/\$$VERSION\$$/$(BUILD_VERSION)/g' pigfx_config.h.in > $(SRC_DIR)/pigfx_config.h
	@echo "Creating pigfx_config.h"

run: pigfx.elf
	./launch_qemu.bash

kernel: pigfx.img
	@echo "Creating kernel.img for Raspberry Pi $(RPI)"
	@cp pigfx.img bin/kernel.img


debug: pigfx.elf
	cd JTAG && ./run_gdb.sh

dump: pigfx.elf
	@$(ARMGNU)-objdump --disassemble-zeroes -D pigfx.elf > pigfx.dump
	@echo "OBJDUMP $<"

$(BUILD_DIR)/%.o : $(SRC_DIR)/%.c 
	@$(ARMGNU)-gcc $(CFLAGS) -c $< -o $@
	@echo "CC $<"

$(BUILD_DIR)/%.o : $(SRC_DIR)/%.S 
	@$(ARMGNU)-gcc $(CFLAGS) -c $< -o $@
	@echo "AS $<"

$(BUILD_DIR)/%.o : $(SRC_DIR)/%.s 
	@$(ARMGNU)-gcc $(CFLAGS) -c $< -o $@
	@echo "AS $<"

%.hex : %.elf 
	@$(ARMGNU)-objcopy $< -O ihex $@
	@echo "OBJCOPY $< -> $@"

%.img : %.elf 
	@$(ARMGNU)-objcopy $< -O binary $@
	@echo "OBJCOPY $< -> $@"

pigfx.elf : $(SRC_DIR)/pigfx_config.h $(OBJS) $(LIBUSPI)
	@$(ARMGNU)-ld $(OBJS) $(LIBGCC) $(LIBUSPI) -T memmap -o $@
	@echo "LD $@"


.PHONY clean :
	rm -f $(SRC_DIR)/pigfx_config.h
	rm -f $(BUILD_DIR)/*.o
	rm -f *.hex
	rm -f *.elf
	rm -f *.img
	rm -f *.dump
	rm -f tags
ifeq ($(strip $(RPI)),4)
	@echo "Skipping USPI clean for Raspberry Pi 4"
else
	@echo "Cleaning USPI"
	@cd uspi && ./makeall clean
endif
