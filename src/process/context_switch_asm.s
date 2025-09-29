# context_switch.s - Assembly implementation of context switching
# This file contains the low-level context switching code for preemptive multitasking

.global context_switch_irq_handler
.global save_current_context 
.global restore_next_context
.global ticks

.extern get_current_process
.extern schedule_next_process
.extern set_current_process

# Offsets for pcb_t structure 
# These must match the actual struct layout - verify with debug_process_table()
.equ PCB_STACK_POINTER, 16      # offsetof(pcb_t, stack_pointer) 
.equ PCB_PAGE_DIRECTORY_PHYS, 32 # offsetof(pcb_t, page_directory_phys)
.equ PCB_IS_USER_PROCESS, 192    # offsetof(pcb_t, is_user_process)

# Timer IRQ handler with context switching
context_switch_irq_handler:
    # Save all registers (full context)
    pushq %rbp
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    # Check if we have a current process
    call get_current_process
    testq %rax, %rax
    jz .no_current_process

    # Save current process context
    # RAX contains current PCB pointer
    movq %rsp, PCB_STACK_POINTER(%rax)  # Save stack pointer to PCB->stack_pointer

.no_current_process:
    # Update timer tick count
    incq ticks(%rip)

    # Call scheduler to get next process  
    call schedule_next_process
    movq %rax, %rbx  # Save next process in RBX
    
    # Set as current process
    movq %rbx, %rdi
    call set_current_process

    # Check if we have a next process
    testq %rbx, %rbx
    jz .no_next_process
    
    # Check if it's a user process - if so, load its address space
    movl PCB_IS_USER_PROCESS(%rbx), %eax
    testl %eax, %eax
    jz .kernel_process
    
    # Load user process address space
    movq PCB_PAGE_DIRECTORY_PHYS(%rbx), %rax
    movq %rax, %cr3
    
.kernel_process:
    # Restore context from new process
    movq PCB_STACK_POINTER(%rbx), %rsp  # Load stack pointer from PCB->stack_pointer
    
    # Send EOI to PIC before restoring context
    movb $0x20, %al
    outb %al, $0x20
    
    # Restore all registers
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax
    popq %rbp
    
    # Return from interrupt (will restore RIP, CS, RFLAGS, and potentially RSP, SS)
    iretq

.no_next_process:
    # No process to switch to, just continue with current interrupt cleanup
    movb $0x20, %al
    outb %al, $0x20
    
    # Restore all registers
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax
    popq %rbp
    
    iretq

# Utility function to save current process context
# Called when a process voluntarily yields
save_current_context:
    # This is called from C, so we need to save caller-saved registers
    # and set up stack frame for the yielding process
    
    # Create interrupt-like frame on stack
    pushfq          # Save RFLAGS
    pushq $8        # Save CS (kernel code segment - 64-bit code segment)
    pushq 8(%rsp)   # Save return address as RIP
    
    # Save all registers
    pushq %rbp
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    
    # Get current process and save stack pointer
    call get_current_process
    testq %rax, %rax
    jz .no_process_to_save
    
    movq %rsp, PCB_STACK_POINTER(%rax)  # Save stack pointer to PCB
    
.no_process_to_save:
    ret

# Utility function to restore next process context  
restore_next_context:
    # RDI contains PCB pointer of process to restore
    movq %rdi, %rbx
    
    # Load address space if user process
    movl PCB_IS_USER_PROCESS(%rbx), %eax
    testl %eax, %eax
    jz .restore_kernel_process
    
    # Load user process address space
    movq PCB_PAGE_DIRECTORY_PHYS(%rbx), %rax
    movq %rax, %cr3
    
.restore_kernel_process:
    # Restore stack pointer
    movq PCB_STACK_POINTER(%rbx), %rsp
    
    # Restore registers
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax
    popq %rbp
    
    # Return using iretq (will restore RIP, CS, RFLAGS)
    iretq

# Global variable for tick count (defined in context_switch.c)
.extern ticks