#include "idt.h"

struct idt_info idt_info_ptr;
struct idt_entry idt[256];

void set_idt_entry(unsigned char isr_index, unsigned int base, unsigned short sel, unsigned char flags) {
    idt[isr_index].offset0_15 = base & 0x0000ffff;
    idt[isr_index].select = sel;
    idt[isr_index].zero = 0;
    idt[isr_index].flags = flags;
    idt[isr_index].offset16_31 = (base & 0xffff0000) >> 16;
}
