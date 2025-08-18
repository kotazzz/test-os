# WSL friendly paths
CC = x86_64-elf-gcc.exe
LD = x86_64-elf-ld.exe
ASM = nasm.exe
QEMU_BIN = qemu-system-x86_64.exe

CFLAGS = -ffreestanding -O2 -Wall -Wextra -mno-red-zone -nostdlib -mcmodel=large -mno-sse
LDFLAGS = -T linker.ld -nostdlib -m elf_x86_64
ASMFLAGS = -f elf64 -w+other

ISO_DIR = iso
ISO_FILE = myos.iso

QEMU_ARGS = -m 16G

all: kernel.elf $(ISO_FILE) run


boot.o: boot.s
	$(ASM) $(ASMFLAGS) boot.s -o boot.o

interrupt.o: interrupt.s
	$(ASM) $(ASMFLAGS) interrupt.s -o interrupt.o

kernel.o: kernel.c
	$(CC) $(CFLAGS) -c kernel.c -o kernel.o

kernel.elf: boot.o interrupt.o kernel.o
	$(LD) $(LDFLAGS) boot.o interrupt.o kernel.o -o kernel.elf

# GRUB ISO
$(ISO_FILE): kernel.elf
	rm -rf $(ISO_DIR)
	mkdir -p $(ISO_DIR)/boot/grub
	mkdir -p $(ISO_DIR)/boot
	cp kernel.elf $(ISO_DIR)/boot/
	cp grub.cfg $(ISO_DIR)/boot/grub/
	grub-mkrescue -o $(ISO_FILE) $(ISO_DIR)


run: $(ISO_FILE)
	$(QEMU_BIN) -cdrom $(ISO_FILE) -serial stdio $(QEMU_ARGS)

clean:
	rm -rf boot.o interrupt.o kernel.o kernel.elf $(ISO_DIR) $(ISO_FILE)