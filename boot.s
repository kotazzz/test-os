%define STACK_SIZE 16384

section .multiboot2
align 8
header_length equ multiboot_header_end - multiboot_header_start

multiboot_header_start:
    dd 0xE85250D6        ; magic
    dd 0                 ; architecture (0 = i386)
    dd header_length
    dd -(0xE85250D6 + 0 + header_length)
    ; End tag
    dw 0
    dw 0
    dd 8
multiboot_header_end:

section .bss
align 16
stack_bottom:
    resb STACK_SIZE
stack_top:

section .text
global _start
extern kmain

_start:
    cli
    mov esp, stack_top    ; set stack pointer
    push ebx
    push eax
    call kmain
halt_loop:
    hlt
    jmp halt_loop