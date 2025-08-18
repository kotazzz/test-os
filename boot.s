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

; Page tables for long mode
align 4096
p4_table:
    resb 4096
p3_table:
    resb 4096
p2_table:
    resb 4096

section .rodata
gdt64:
    dq 0x0000000000000000    ; null descriptor
.code: equ $ - gdt64
    dq 0x00209A0000000000    ; 64-bit code segment (L=1, P=1, DPL=0, Type=1010)
.data: equ $ - gdt64  
    dq 0x0000920000000000    ; 64-bit data segment
.pointer:
    dw $ - gdt64 - 1         ; length
    dq gdt64                 ; address

section .text
bits 32
global _start
extern kmain

_start:
    cli
    mov esp, stack_top

    ; Check multiboot2
    cmp eax, 0x36d76289
    jne .no_multiboot
    
    ; Check CPUID support
    call check_cpuid
    call check_long_mode
    
    ; Set up paging
    call setup_page_tables
    call enable_paging
    
    ; Load GDT
    lgdt [gdt64.pointer]
    
    ; Jump to long mode with far jump
    jmp gdt64.code:long_mode_start
    
.no_multiboot:
    mov al, "0"
    jmp error

check_cpuid:
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    xor eax, ecx
    jz .no_cpuid
    ret
.no_cpuid:
    mov al, "1"
    jmp error

check_long_mode:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode
    
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .no_long_mode
    ret
.no_long_mode:
    mov al, "2"
    jmp error

setup_page_tables:
    ; Map first P4 entry to P3 table
    mov eax, p3_table
    or eax, 0b11        ; present + writable
    mov [p4_table], eax
    
    ; Map first P3 entry to P2 table
    mov eax, p2_table
    or eax, 0b11        ; present + writable
    mov [p3_table], eax
    
    ; Map each P2 entry to a huge 2MiB page
    mov ecx, 0
.map_p2_table:
    mov eax, 0x200000
    mul ecx
    or eax, 0b10000011  ; present + writable + huge
    mov [p2_table + ecx * 8], eax
    inc ecx
    cmp ecx, 512
    jne .map_p2_table
    ret

enable_paging:
    ; Load P4 to cr3 register
    mov eax, p4_table
    mov cr3, eax
    
    ; Enable PAE flag in cr4
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax
    
    ; Set the long mode bit in EFER MSR
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr
    
    ; Enable paging in cr0 register
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax
    ret

error:
    mov dword [0xb8000], 0x4f524f45
    mov dword [0xb8004], 0x4f3a4f52
    mov dword [0xb8008], 0x4f204f20
    mov byte [0xb800a], al
    hlt

bits 64
long_mode_start:
    ; Load data segment registers
    mov ax, gdt64.data
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Set up stack
    mov rsp, stack_top
    
    ; Call kernel
    call kmain
    
.halt:
    hlt
    jmp .halt