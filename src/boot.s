#define STACK_SIZE 16384

.section .multiboot2, "a"
.align 8
multiboot_header_start:
    .long 0xE85250D6        # magic
    .long 0                 # architecture (0 = i386)
    .long multiboot_header_end - multiboot_header_start # header_length
    .long -(0xE85250D6 + 0 + (multiboot_header_end - multiboot_header_start)) # checksum
    # End tag
    .word 0
    .word 0
    .long 8
multiboot_header_end:

.section .bss
.align 16
stack_bottom:
    .space 16384
stack_top:

# Page tables for long mode
.align 4096
p4_table:
    .space 4096
p3_table:
    .space 4096
p2_table:
    .space 4096

.section .rodata
gdt64:
    .quad 0x0000000000000000    # null descriptor
gdt64_code:
    .quad 0x00209A0000000000    # 64-bit code segment (L=1, P=1, DPL=0, Type=1010)
gdt64_data:
    .quad 0x0000920000000000    # 64-bit data segment
gdt64_pointer:
    .word . - gdt64 - 1         # length
    .quad gdt64                 # address

.section .text
.code32
.global _start
.extern kmain

_start:
    cli
    movl $stack_top, %esp

    # Save multiboot2 info pointer from EBX
    movl %ebx, %edi  # Save MBI pointer in EDI (preserved across calls)

    # Check multiboot2
    cmpl $0x36d76289, %eax
    jne .no_multiboot

    # Check CPUID support
    call check_cpuid
    call check_long_mode

    # Set up paging
    call setup_page_tables
    call enable_paging

    # Load GDT
    lgdt gdt64_pointer

    # Jump to long mode with far jump
    ljmp $8, $long_mode_start

.no_multiboot:
    movb $'0', %al
    jmp error

check_cpuid:
    pushf
    popl %eax
    movl %eax, %ecx
    xorl $(1 << 21), %eax
    pushl %eax
    popf
    pushf
    popl %eax
    pushl %ecx
    popf
    xorl %ecx, %eax
    jz .no_cpuid
    ret
.no_cpuid:
    movb $'1', %al
    jmp error

check_long_mode:
    movl $0x80000000, %eax
    cpuid
    cmpl $0x80000001, %eax
    jb .no_long_mode

    movl $0x80000001, %eax
    cpuid
    testl $(1 << 29), %edx
    jz .no_long_mode
    ret
.no_long_mode:
    movb $'2', %al
    jmp error

setup_page_tables:
    # Map first P4 entry to P3 table
    movl $p3_table, %eax
    orl $0b11, %eax        # present + writable
    movl %eax, p4_table

    # Map first P3 entry to P2 table
    movl $p2_table, %eax
    orl $0b11, %eax        # present + writable
    movl %eax, p3_table

    # Map each P2 entry to a huge 2MiB page
    movl $0, %ecx
.map_p2_table:
    movl $0x200000, %eax
    mull %ecx
    orl $0b10000011, %eax  # present + writable + huge
    movl %eax, p2_table(,%ecx,8)
    incl %ecx
    cmpl $512, %ecx
    jne .map_p2_table
    ret

enable_paging:
    # Load P4 to cr3 register
    movl $p4_table, %eax
    movl %eax, %cr3

    # Enable PAE flag in cr4
    movl %cr4, %eax
    orl $(1 << 5), %eax
    movl %eax, %cr4

    # Set the long mode bit in EFER MSR
    movl $0xC0000080, %ecx
    rdmsr
    orl $(1 << 8), %eax
    wrmsr

    # Enable paging in cr0 register
    movl %cr0, %eax
    orl $(1 << 31), %eax
    movl %eax, %cr0
    ret

error:
    movl $0x4f524f45, 0xb8000
    movl $0x4f3a4f52, 0xb8004
    movl $0x4f204f20, 0xb8008
    movb %al, 0xb800a
    hlt

.code64
long_mode_start:
    # Load data segment registers
    movw $16, %ax
    movw %ax, %ss
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    # Set up stack
    movq $stack_top, %rsp

    # Pass multiboot2 info pointer as first argument (RDI in x86-64 calling convention)
    # RDI already contains the value from EDI
    
    # Call kernel
    call kmain

.halt:
    hlt
    jmp .halt
