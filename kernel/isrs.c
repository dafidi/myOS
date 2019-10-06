#include "isrs.h"
#include "system.h"

#include "../drivers/screen.h"

static char* exception_messages[] = {
  "Division By Zero Exception",
  "Debug Exception",
  "Non Maskable Interrupt Exception",
  "Breakpoint Exception",
  "Into Detected Overflow Exception",
  "Out of Bounds Exception",
  "Invalid Opcode Exception",
  "No Coprocessor Exception",
  "Double Fault Exception ",
  "Coprocessor Segment Overrun Exception",
  "Bad TSS Exception ",
  "Segment Not Present Exception ",
  "Stack Fault Exception ",
  "General Protection Fault Exception ",
  "Page Fault Exception ",
  "Unknown Interrupt Exception",
  "Coprocessor Fault Exception",
  "Alignment Check Exception (486+)",
  "Machine Check Exception (Pentium/586+)"
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved"
};

#define mine_
void install_isrs() {
#ifdef mine
  set_idt_entry(0, (unsigned) isr0, 0x08, 0xE1);
  set_idt_entry(1, (unsigned) isr1, 0x08, 0xE1);
  set_idt_entry(2, (unsigned) isr2, 0x08, 0xE1);
  set_idt_entry(3, (unsigned) isr3, 0x08, 0xE1);
  set_idt_entry(4, (unsigned) isr4, 0x08, 0xE1);
  set_idt_entry(5, (unsigned) isr5, 0x08, 0xE1);
  set_idt_entry(6, (unsigned) isr6, 0x08, 0xE1);
  set_idt_entry(7, (unsigned) isr7, 0x08, 0xE1);
  set_idt_entry(8, (unsigned) isr8, 0x08, 0xE1);
  set_idt_entry(9, (unsigned) isr9, 0x08, 0xE1);
  set_idt_entry(10, (unsigned) isr10, 0x08, 0xE1);
  set_idt_entry(11, (unsigned) isr11, 0x08, 0xE1);
  set_idt_entry(12, (unsigned) isr12, 0x08, 0xE1);
  set_idt_entry(13, (unsigned) isr13, 0x08, 0xE1);
  set_idt_entry(14, (unsigned) isr14, 0x08, 0xE1);
  set_idt_entry(15, (unsigned) isr15, 0x08, 0xE1);
  set_idt_entry(16, (unsigned) isr16, 0x08, 0xE1);
  set_idt_entry(17, (unsigned) isr17, 0x08, 0xE1);
  set_idt_entry(18, (unsigned) isr18, 0x08, 0xE1);
  set_idt_entry(19, (unsigned) isr19, 0x08, 0xE1);
  set_idt_entry(20, (unsigned) isr20, 0x08, 0xE1);
  set_idt_entry(21, (unsigned) isr21, 0x08, 0xE1);
  set_idt_entry(22, (unsigned) isr22, 0x08, 0xE1);
  set_idt_entry(23, (unsigned) isr23, 0x08, 0xE1);
  set_idt_entry(24, (unsigned) isr24, 0x08, 0xE1);
  set_idt_entry(25, (unsigned) isr25, 0x08, 0xE1);
  set_idt_entry(26, (unsigned) isr26, 0x08, 0xE1);
  set_idt_entry(27, (unsigned) isr27, 0x08, 0xE1);
  set_idt_entry(28, (unsigned) isr28, 0x08, 0xE1);
  set_idt_entry(29, (unsigned) isr29, 0x08, 0xE1);
  set_idt_entry(30, (unsigned) isr30, 0x08, 0xE1);
  set_idt_entry(31, (unsigned) isr31, 0x08, 0xE1);
#else
  set_idt_entry(0, (unsigned) isr0, 0x08, 0x8E);
  set_idt_entry(1, (unsigned) isr1, 0x08, 0x8E);
  set_idt_entry(2, (unsigned) isr2, 0x08, 0x8E);
  set_idt_entry(3, (unsigned) isr3, 0x08, 0x8E);
  set_idt_entry(4, (unsigned) isr4, 0x08, 0x8E);
  set_idt_entry(5, (unsigned) isr5, 0x08, 0x8E);
  set_idt_entry(6, (unsigned) isr6, 0x08, 0x8E);
  set_idt_entry(7, (unsigned) isr7, 0x08, 0x8E);
  set_idt_entry(8, (unsigned) isr8, 0x08, 0x8E);
  set_idt_entry(9, (unsigned) isr9, 0x08, 0x8E);
  set_idt_entry(10, (unsigned) isr10, 0x08, 0x8E);
  set_idt_entry(11, (unsigned) isr11, 0x08, 0x8E);
  set_idt_entry(12, (unsigned) isr12, 0x08, 0x8E);
  set_idt_entry(13, (unsigned) isr13, 0x08, 0x8E);
  set_idt_entry(14, (unsigned) isr14, 0x08, 0x8E);
  set_idt_entry(15, (unsigned) isr15, 0x08, 0x8E);
  set_idt_entry(16, (unsigned) isr16, 0x08, 0x8E);
  set_idt_entry(17, (unsigned) isr17, 0x08, 0x8E);
  set_idt_entry(18, (unsigned) isr18, 0x08, 0x8E);
  set_idt_entry(19, (unsigned) isr19, 0x08, 0x8E);
  set_idt_entry(20, (unsigned) isr20, 0x08, 0x8E);
  set_idt_entry(21, (unsigned) isr21, 0x08, 0x8E);
  set_idt_entry(22, (unsigned) isr22, 0x08, 0x8E);
  set_idt_entry(23, (unsigned) isr23, 0x08, 0x8E);
  set_idt_entry(24, (unsigned) isr24, 0x08, 0x8E);
  set_idt_entry(25, (unsigned) isr25, 0x08, 0x8E);
  set_idt_entry(26, (unsigned) isr26, 0x08, 0x8E);
  set_idt_entry(27, (unsigned) isr27, 0x08, 0x8E);
  set_idt_entry(28, (unsigned) isr28, 0x08, 0x8E);
  set_idt_entry(29, (unsigned) isr29, 0x08, 0x8E);
  set_idt_entry(30, (unsigned) isr30, 0x08, 0x8E);
  set_idt_entry(31, (unsigned) isr31, 0x08, 0x8E);
#endif
}
void  int_to_string(char* s, int v);

void fault_handler(struct registers* regs) {
  print("Handling Fault.\n");
  char* s = "0000";
  int_to_string(s, regs->int_no);
  print("int_no: ");
  print(s);
  print("\n");
  if (regs->int_no < 32) {
    print(exception_messages[regs->int_no]);
  }
  for(;;);
  print("Fault Handled.\n");
}

void int_to_string(char* s, int val) {
  const int size = 5;
  char t;
  for (int i = 0; i < size && val; i++) {
    t = val % 10;
    s[size - i - 1] = t + 48;
    val /= 10;
  }
}
