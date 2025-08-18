ENTRY(_start)

SECTIONS
{
    . = 1M;

    /* Keep multiboot2 header in ELF */
    .multiboot2 : ALIGN(8) { KEEP(*(.multiboot2)) }

    .text : ALIGN(4096) { *(.text*) }
    .rodata : ALIGN(4096) { *(.rodata*) }
    .data : ALIGN(4096) { *(.data*) }
    .bss  : ALIGN(4096) { *(.bss*) *(COMMON) }
}