#include "idt.h"

#include "system.h"

#ifdef CONFIG32
extern void asm_initialize_idt(void);
#else
extern void asm_initialize_idt64(void);
#endif

struct idt_info idt_info_ptr;
struct idt_entry idt[256];
struct idt64_entry idt64[256];

void set_idt_entry(unsigned char isr_index, unsigned int base, unsigned short sel, unsigned char flags) {
    idt[isr_index].offset0_15 = base & 0x0000ffff;
    idt[isr_index].select = sel;
    idt[isr_index].zero = 0;
    idt[isr_index].flags = flags;
    idt[isr_index].offset16_31 = (base & 0xffff0000) >> 16;
}

void set_idt64_entry(unsigned int isr_index, uint64_t base, unsigned short sel, unsigned char ist, unsigned char type_flags) {
    idt64[isr_index].offset0_15 = base & 0x0000ffff;
    idt64[isr_index].select = sel;
    idt64[isr_index].ist = ist & 0x07;
    idt64[isr_index].type_flags = type_flags;
    idt64[isr_index].offset16_31 = (base & 0xffff0000) >> 16;
    idt64[isr_index].offset32_63 = (base & 0xffffffff00000000ULL) >> 32;
}

void initialize_idt(void) {
#ifdef CONFIG32
    asm_initialize_idt();
#else
    asm_initialize_idt64();
#endif
}
