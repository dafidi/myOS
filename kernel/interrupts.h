#ifndef __INTERRUPTS_H__
#define __INTERRUPTS_H__

#include "idt.h"
#include "irq.h"
#include "isrs.h"
#include "system.h"

extern void enable_interrupts(void);
extern void enable_interrupts64(void);
extern void initialize_idt(void);

#define NUM_REGISTERED_IRQS 48

extern struct idt_info idt_info_ptr;
extern struct idt_entry idt[256];
extern struct idt64_entry idt64[256];

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
}

static void start_interrupts(void) {
    enable_interrupts64();
}

#endif // __INTERRUPTS_H__
