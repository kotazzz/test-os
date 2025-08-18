# WSL friendly paths
CC = x86_64-elf-gcc.exe
LD = x86_64-elf-ld.exe
ASM = nasm.exe
QEMU_BIN = qemu-system-x86_64.exe

CFLAGS = -ffreestanding -O2 -Wall -Wextra -mno-red-zone -nostdlib -m32
LDFLAGS = -T linker.ld -nostdlib -m elf_i386
ASMFLAGS = -f elf32

ISO_DIR = iso
ISO_FILE = myos.iso

all: kernel.elf $(ISO_FILE) run


boot.o: boot.s
	$(ASM) $(ASMFLAGS) boot.s -o boot.o

kernel.o: kernel.c
	$(CC) $(CFLAGS) -c kernel.c -o kernel.o

kernel.elf: boot.o kernel.o
	$(LD) $(LDFLAGS) boot.o kernel.o -o kernel.elf

# GRUB ISO
$(ISO_FILE): kernel.elf
	rm -rf $(ISO_DIR)
	mkdir -p $(ISO_DIR)/boot/grub
	mkdir -p $(ISO_DIR)/boot
	cp kernel.elf $(ISO_DIR)/boot/
	cp grub.cfg $(ISO_DIR)/boot/grub/
	grub-mkrescue -o $(ISO_FILE) $(ISO_DIR)


run: $(ISO_FILE)
	$(QEMU_BIN) -cdrom $(ISO_FILE) -serial stdio

clean:
	rm -rf boot.o kernel.o kernel.elf $(ISO_DIR) $(ISO_FILE)