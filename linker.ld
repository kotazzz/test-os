ENTRY(_start)

SECTIONS
{
    . = 1M;

    /* Keep multiboot2 header in ELF */
    .multiboot2 : { KEEP(*(.multiboot2)) }

    .text : { *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss  : { *(.bss*) *(COMMON) }
}