.section .text
.global _start
.code64

/*
 * kronox64 - Simple microkernel bootstrap for x86_64
 * 
 * - Setup 64-bit GDT (code and data segments)
 * - Setup a basic IDT with a divide-by-zero handler
 * - Enable interrupts
 * - Main loop waits for interrupts (hlt instruction)
 * 
 * GNU assembler syntax.
 */

_start:
    # Load GDT
    lea gdt64_desc(%rip), %rax
    lgdt (%rax)

    # Enable 64-bit mode (Long Mode)
    # Read EFER MSR (Model Specific Register)
    mov $0x0000080, %ecx
    rdmsr
    or $0x100, %eax            # Enable LME (Long Mode Enable) bit
    wrmsr

    # Set CR4 to enable PAE (bit 5)
    mov %cr4, %rax
    or $0x20, %rax
    mov %rax, %cr4

    # Set CR0 to enable protected mode and paging
    mov %cr0, %rax
    or $0x80000001, %rax
    mov %ra, %cr0

    # Far jump to reload CS with 64-bit code segment selector
    pushq $0x08                # Code segment selector (GDT entry 1)
    lea 1f(%rip), %rax
    pushq %rax
    lretq                      # Far return to switch CS and enter 64-bit mode

1: 
    # We are now in 64-bit mode
    # Load data selectors into DS, ES, SS
    mov $0x10, %ax             # Data segment selector (GDT entry 2)
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss

    # Load IDT
    lea idt_desc(%rip), %rax
    lidt (%rax)

    # Enable interrupts globally
    sti

    # Main loop - halt CPU unil next interrupt, then loop again
main_loop:
    hlt
    jmp main_loop

# --- Divide-by-xero exception handler ---

divide_by_zero_handler:
    # Save registers (only a few hee for simplicity)
    push %rax
    push %rcx
    push %rdx

    # Infinite loop to halt CPU when divide-by-zero occurs
zero_loop:
    hlt
    jmp zero_loop

    # Restore registers (should never reach here)
    pop %rdx 
    pop %rcx
    pop %rax
    iretq

# --- GDT table definitions ---

.align 8
gdt64:
    .quad 0x0000000000000000   # Null descriptor
    .quad 0x00af9a000000ffff   # Code segment descriptor, 64-bit code segment
    .quad 0x00af92000000ffff   # Data segment descriptor

gdt64_end:

gdt64_desc:
     .word gdt64_end - gdt64 - 1
     .quad gdt64

# -- IDT table definitions ---

.align 16
idt:
    # Entry 0: Divide-by-zero interrupt vector
    .word (divide_by_zero_handler & 0xFFFF)           # offset bits 0..15
    .word 0x08                                        # selector (code segment)
    .byte 0                                          # zero
    .byte 0x8E                                       # flags: present, DPL=0, type=interrupt gate
    .word ((divide_by_zero_handler >> 16) & 0xFFFF)  # offset bits 16..31
    .long (divide_by_zero_handler >> 32)             # offset bits 32..63
    .long 0                                          # reserved

    # Remaining IDT entries zeroed out
    .quad 0
    .quad 0

idt_end:

idt_desc:
    .word idt_end - idt - 1
    .quad idt
