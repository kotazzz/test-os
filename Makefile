# Toolchain
CC       = x86_64-elf-gcc.exe
CXX     := x86_64-elf-g++.exe
LD       = x86_64-elf-ld.exe
ASM      = nasm.exe
QEMU     = qemu-system-x86_64.exe

# Flags
CFLAGS   = -ffreestanding -O2 -Wall -Wextra -mno-red-zone -nostdlib -mcmodel=large -mno-sse -MMD -MP
CXXFLAGS:= $(CFLAGS) -fno-exceptions -fno-rtti
LDFLAGS  = -T linker.ld -nostdlib -m elf_x86_64
ASMFLAGS = -f elf64 -w+other

# Directories
SRC      = src
BUILD    = build
ISO_DIR  = iso
DIST     = dist
ISO_FILE = $(DIST)/myos.iso

# Sources
CSRC    := $(shell find $(SRC) -name '*.c')
CPPSRC  := $(shell find $(SRC) -name '*.cpp')
ASMSRC  := $(shell find $(SRC) -name '*.s')

OBJS    := $(patsubst $(SRC)/%, $(BUILD)/%, \
              $(CSRC:.c=.o) $(CPPSRC:.cpp=.o) $(ASMSRC:.s=.o))


# QEMU
QEMU_ARGS = -serial stdio

.PHONY: all clean run iso

all: $(BUILD)/kernel.elf iso run

# C
$(BUILD)/%.o: $(SRC)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# C++
$(BUILD)/%.o: $(SRC)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ASM
$(BUILD)/%.o: $(SRC)/%.s
	@mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS) $< -o $@

# Линковка
$(BUILD)/kernel.elf: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

iso: $(BUILD)/kernel.elf
	@mkdir -p $(DIST)
	rm -rf $(ISO_DIR)
	mkdir -p $(ISO_DIR)/boot/grub
	cp $< $(ISO_DIR)/boot/
	cp grub.cfg $(ISO_DIR)/boot/grub/
	grub-mkrescue -o $(ISO_FILE) $(ISO_DIR)

run: iso
	$(QEMU) -cdrom $(ISO_FILE) $(QEMU_ARGS)

clean:
	rm -rf $(BUILD) $(DIST) $(ISO_DIR)

-include $(OBJS:.o=.d)