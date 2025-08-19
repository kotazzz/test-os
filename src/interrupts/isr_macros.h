#ifndef ISR_MACROS_H
#define ISR_MACROS_H

// Макрос для генерации assembly заглушки IRQ в отдельном файле
#define GENERATE_IRQ_HANDLER(num) \
__asm__( \
    ".global irq" #num "_handler\n" \
    "irq" #num "_handler:\n" \
    "    SAVE_CONTEXT\n" \
    "    movq $" #num ", %rdi\n" \
    "    call isr_handler\n" \
    "    RESTORE_CONTEXT\n" \
    "    iretq\n" \
);

// Макрос для регистрации обработчика IRQ
#define REGISTER_IRQ_HANDLER(irq_num, handler_func) \
    register_interrupt_handler(32 + irq_num, handler_func)

#endif
