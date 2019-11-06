#include "system.h"
#include "isrs.h"

#include "drivers/screen/screen.h"

void install_isrs() {
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
}

  char* exception_messages[] = {
    /*0 */ "Division By Zero Exception",
    /*1 */ "Debug Exception",
    /*2 */ "Non Maskable Interrupt Exception",
    /*3 */ "Breakpoint Exception",
    /*4 */ "Into Detected Overflow Exception",
    /*5 */ "Out of Bounds Exception",
    /*6 */ "Invalid Opcode Exception",
    /*7 */ "No Coprocessor Exception",
    /*8 */ "Double Fault Exception ",
    /*9 */ "Coprocessor Segment Overrun Exception",
    /*10*/  "Bad TSS Exception ",
    /*11*/  "Segment Not Present Exception ",
    /*12*/  "Stack Fault Exception ",
    /*13*/  "General Protection Fault Exception ",
    /*14*/  "Page Fault Exception ",
    /*15*/  "Unknown Interrupt Exception",
    /*16*/  "Coprocessor Fault Exception",
    /*17*/  "Alignment Check Exception (486+)",
    /*18*/  "Machine Check Exception (Pentium/586+)"
    /*19*/  "Reserved",
    /*20*/  "Reserved",
    /*21*/  "Reserved",
    /*22*/  "Reserved",
    /*23*/  "Reserved",
    /*24*/  "Reserved",
    /*25*/  "Reserved",
    /*26*/  "Reserved",
    /*27*/  "Reserved",
    /*28*/  "Reserved",
    /*29*/  "Reserved",
    /*30*/  "Reserved"
  };

void fault_handler(struct registers* regs) {
  print("Handling Fault.\n");
  print(exception_messages[regs->int_no]);
  print("\n");
  return;
}
