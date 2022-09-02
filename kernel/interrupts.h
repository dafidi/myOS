#ifndef __INTERRUPTS_H__
#define __INTERRUPTS_H__

#include "idt.h"
#include "irq.h"
#include "isrs.h"

extern void enable_interrupts(void);
extern void initialize_idt(void);

static void init_interrupts(void) {
    idt_info_ptr.base = (u32) (&idt[0]);
    idt_info_ptr.limit = sizeof(struct idt_entry) * 256;

    initialize_idt();

    install_isrs();
    install_irqs();
}

static void start_interrupts(void) {
    enable_interrupts();
}

#endif // __INTERRUPTS_H__
