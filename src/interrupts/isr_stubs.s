[BITS 64]
section .text

; Макрос для сохранения контекста
%macro SAVE_CONTEXT 0
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

; Макрос для восстановления контекста
%macro RESTORE_CONTEXT 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro

; Макрос для исключений БЕЗ error code
%macro ISR_STUB_NO_ERR 2
global %1
%1:
    push 0               ; Push dummy error code для выравнивания стека
    SAVE_CONTEXT
    mov rdi, %2          ; Передаем номер прерывания
    extern isr_handler
    call isr_handler
    RESTORE_CONTEXT
    add rsp, 8           ; Убираем dummy error code
    iretq
%endmacro

; Макрос для исключений С error code
%macro ISR_STUB_ERR 2
global %1
%1:
    SAVE_CONTEXT         ; Error code уже в стеке
    mov rdi, %2
    extern isr_handler
    call isr_handler
    RESTORE_CONTEXT
    add rsp, 8           ; Убираем error code
    iretq
%endmacro

; Макрос для IRQ (всегда без error code)
%macro IRQ_STUB 2
global %1
%1:
    SAVE_CONTEXT
    mov rdi, %2
    extern isr_handler
    call isr_handler
    RESTORE_CONTEXT
    iretq
%endmacro

; Исключения БЕЗ error code
ISR_STUB_NO_ERR isr0, 0    ; Division by Zero
ISR_STUB_NO_ERR isr1, 1    ; Debug
ISR_STUB_NO_ERR isr2, 2    ; NMI
ISR_STUB_NO_ERR isr3, 3    ; Breakpoint
ISR_STUB_NO_ERR isr4, 4    ; Overflow
ISR_STUB_NO_ERR isr5, 5    ; Bound Range Exceeded
ISR_STUB_NO_ERR isr6, 6    ; Invalid Opcode
ISR_STUB_NO_ERR isr7, 7    ; Device Not Available

; Исключения С error code
ISR_STUB_ERR isr8, 8       ; Double Fault
ISR_STUB_NO_ERR isr9, 9    ; Coprocessor Segment Overrun
ISR_STUB_ERR isr10, 10     ; Invalid TSS
ISR_STUB_ERR isr11, 11     ; Segment Not Present
ISR_STUB_ERR isr12, 12     ; Stack Segment Fault
ISR_STUB_ERR isr13, 13     ; General Protection Fault
ISR_STUB_ERR isr14, 14     ; Page Fault
ISR_STUB_NO_ERR isr15, 15  ; Reserved
ISR_STUB_NO_ERR isr16, 16  ; x87 FPU Exception
ISR_STUB_ERR isr17, 17     ; Alignment Check
ISR_STUB_NO_ERR isr18, 18  ; Machine Check
ISR_STUB_NO_ERR isr19, 19  ; SIMD Exception

; IRQ handlers
IRQ_STUB irq0_handler, 32  ; Timer
IRQ_STUB irq1_handler, 33  ; Keyboard

; Старый обработчик для совместимости
ISR_STUB_NO_ERR isr_stub_default, 0
