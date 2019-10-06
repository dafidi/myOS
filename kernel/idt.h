#ifndef __IDT_H__
#define __IDT_H__

#define u16 unsigned short
#define u8 unsigned char
#define u32 unsigned int

struct idt_entry {
  u16 offset0_15;
  u16 select;
  u8 zero;
  u8 flags;
  u16 offset16_31;
}__attribute__((packed));

struct idt_info {
  u16 limit;
  u32 base;
}__attribute__((packed));

struct idt_info idt_info_ptr;
struct idt_entry idt[256];

void init_idt(void);
void set_idt_entry(unsigned char isr_index, unsigned int base, unsigned short sel, unsigned char flags);

#endif // __IDT_H__
