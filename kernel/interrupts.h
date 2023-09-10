#ifndef __INTERRUPTS_H__
#define __INTERRUPTS_H__

#include "idt.h"
#include "irq.h"
#include "isrs.h"
#include "system.h"

extern void enable_interrupts(void);
extern void asm_enable_interrupts64(void);
extern void initialize_idt(void);

#define NUM_REGISTERED_IRQS 48

extern const pa_t _bss_end;

extern struct idt_info idt_info_ptr;
extern struct idt_entry idt[256];
extern struct idt64_entry idt64[256];

extern pa_t _interrupt_stacks_begin;
extern pa_t _interrupt_stacks_end;
extern pa_t _interrupt_stacks_length;

static void init_interrupts(void) {

#ifdef CONFIG32
    idt_info_ptr.base = addr_to_u32(&idt[0]);
    idt_info_ptr.limit = sizeof(struct idt_entry) * NUM_REGISTERED_IRQS - 1;
#else
    idt_info_ptr.base = addr_to_u32(&idt64[0]);
    idt_info_ptr.limit = sizeof(struct idt64_entry) * NUM_REGISTERED_IRQS - 1;
#endif

    install_isrs();
    install_irqs();

    initialize_idt();

    // Place the interrupt stacks right after the kernel stack.
    // Add PAGE_SIZE to create some room between dynamic memory pool
    // and interrupt stacks.
    _interrupt_stacks_begin = PAGE_ALIGN_UP(_bss_end + KERNEL_STACK_SIZE + PAGE_SIZE);
    _interrupt_stacks_end = _interrupt_stacks_begin + 0x7000;
    _interrupt_stacks_length = _interrupt_stacks_end - _interrupt_stacks_begin;
}

static void start_interrupts(void) {
    asm_enable_interrupts64();
}

#endif // __INTERRUPTS_H__
