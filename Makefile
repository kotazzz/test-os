
CC       := gcc
CXX      := g++
LD       := ld
QEMU     := qemu-system-x86_64
GRUB     := grub-mkrescue

CFLAGS   := -ffreestanding -O2 -Wall -Wextra -mno-red-zone -nostdlib -mcmodel=large -mno-sse -MMD -MP -Isrc
CXXFLAGS := $(CFLAGS) -fno-exceptions -fno-rtti
LDFLAGS  := -T linker.ld -nostdlib -m elf_x86_64
QEMU_OPTS := -m 256M

SRC_DIR  := src
BUILD_DIR:= build
DIST_DIR := dist
ISO_DIR  := iso

KERNEL   := $(BUILD_DIR)/kernel.elf
ISO_FILE := $(DIST_DIR)/myos.iso

C_SOURCES    := $(shell /usr/bin/find $(SRC_DIR) -name '*.c')
CPP_SOURCES  := $(shell /usr/bin/find $(SRC_DIR) -name '*.cpp')
ASM_SOURCES  := $(shell /usr/bin/find $(SRC_DIR) -name '*.s')

C_OBJS   := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
CPP_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CPP_SOURCES))
ASM_OBJS := $(patsubst $(SRC_DIR)/%.s,$(BUILD_DIR)/%.o,$(ASM_SOURCES))

.PHONY: all run iso clean

all: $(ISO_FILE) run

$(ISO_FILE): $(KERNEL) grub.cfg
	@mkdir -p $(DIST_DIR)
	@rm -rf $(ISO_DIR)
	@mkdir -p $(ISO_DIR)/boot/grub
	cp $< $(ISO_DIR)/boot/kernel.elf
	cp grub.cfg $(ISO_DIR)/boot/grub/
	$(GRUB) -o $(ISO_FILE) $(ISO_DIR)
	
# Сборка ядра
$(KERNEL): $(C_OBJS) $(CPP_OBJS) $(ASM_OBJS)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $^

# Компиляция C
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Компиляция C++
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Ассемблер (использует GCC для совместимости с AT&T синтаксисом)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.s
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Запуск в QEMU
run: $(ISO_FILE)
	$(QEMU) -cdrom $(ISO_FILE) -serial stdio $(QEMU_OPTS)

# Запуск в QEMU в режиме без GUI (для тестирования)
test: $(ISO_FILE)
	$(QEMU) -cdrom $(ISO_FILE) -nographic $(QEMU_OPTS)


# Очистка
clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR) $(ISO_DIR)
