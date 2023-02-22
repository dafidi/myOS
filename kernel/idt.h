#ifndef __IDT_H__
#define __IDT_H__

#include "system.h"

struct idt_entry {
    uint16_t offset0_15;
    uint16_t select;
    uint8_t zero;
    uint8_t flags;
    uint16_t offset16_31;
}__attribute__((packed));

struct idt64_entry {
    uint16_t offset0_15;
    uint16_t select;
    uint8_t ist;
    uint8_t type_flags;
    uint16_t offset16_31;
    uint32_t offset32_63;
    uint32_t reserved;
}__attribute__((packed));

struct idt_info {
    uint16_t limit;
    uint32_t base;
}__attribute__((packed));

void init_idt(void);
void set_idt_entry(unsigned char isr_index, unsigned int base, unsigned short sel, unsigned char flags);
void set_idt64_entry(unsigned int index, uint64_t base, unsigned short sel, unsigned char ist, unsigned char flags);

#endif // __IDT_H__
