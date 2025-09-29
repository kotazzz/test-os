.code64
.section .text

# Макрос для сохранения контекста
.macro SAVE_CONTEXT
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %rbp
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
.endm

# Макрос для восстановления контекста
.macro RESTORE_CONTEXT
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rbp
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax
.endm

# Макрос для исключений БЕЗ error code
.macro ISR_STUB_NO_ERR name, num
.global \name
\name:
    pushq $0               # Push dummy error code для выравнивания стека
    SAVE_CONTEXT
    movq $\num, %rdi       # Передаем номер прерывания
    call isr_handler
    RESTORE_CONTEXT
    addq $8, %rsp          # Убираем dummy error code
    iretq
.endm

# Макрос для исключений С error code
.macro ISR_STUB_ERR name, num
.global \name
\name:
    SAVE_CONTEXT           # Error code уже в стеке
    movq $\num, %rdi
    call isr_handler
    RESTORE_CONTEXT
    addq $8, %rsp          # Убираем error code
    iretq
.endm

# Макрос для IRQ (всегда без error code)
.macro IRQ_STUB name, num
.global \name
\name:
    SAVE_CONTEXT
    movq $\num, %rdi
    call isr_handler
    RESTORE_CONTEXT
    iretq
.endm

# Исключения БЕЗ error code
ISR_STUB_NO_ERR isr0, 0    # Division by Zero
ISR_STUB_NO_ERR isr1, 1    # Debug
ISR_STUB_NO_ERR isr2, 2    # NMI
ISR_STUB_NO_ERR isr3, 3    # Breakpoint
ISR_STUB_NO_ERR isr4, 4    # Overflow
ISR_STUB_NO_ERR isr5, 5    # Bound Range Exceeded
ISR_STUB_NO_ERR isr6, 6    # Invalid Opcode
ISR_STUB_NO_ERR isr7, 7    # Device Not Available

# Исключения С error code
ISR_STUB_ERR isr8, 8       # Double Fault
ISR_STUB_NO_ERR isr9, 9    # Coprocessor Segment Overrun
ISR_STUB_ERR isr10, 10     # Invalid TSS
ISR_STUB_ERR isr11, 11     # Segment Not Present
ISR_STUB_ERR isr12, 12     # Stack Segment Fault
ISR_STUB_ERR isr13, 13     # General Protection Fault
ISR_STUB_ERR isr14, 14     # Page Fault
ISR_STUB_NO_ERR isr15, 15  # Reserved
ISR_STUB_NO_ERR isr16, 16  # x87 FPU Exception
ISR_STUB_ERR isr17, 17     # Alignment Check
ISR_STUB_NO_ERR isr18, 18  # Machine Check
ISR_STUB_NO_ERR isr19, 19  # SIMD Exception

# IRQ handlers
IRQ_STUB irq0_handler, 32  # Timer
IRQ_STUB irq1_handler, 33  # Keyboard

# Старый обработчик для совместимости
ISR_STUB_NO_ERR isr_stub_default, 0
