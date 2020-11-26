#include "system.h"
#include "low_level.h"

#include <drivers/screen/screen.h>

#include "irq.h"
#include "idt.h"

#define PIC_INIT_MSG 0x11
#define PIC_MASTER_CMD_PORT 0x20
#define PIC_SLAVE_CMD_PORT 0xA0
#define PIC_MASTER_DATA_PORT 0x21
#define PIC_SLAVE_DATA_PORT 0xA1
#define PIC_MASTER_IDT_OFFSET 0x20
#define PIC_SLAVE_IDT_OFFSET 0x28
#define PIC_EOI

void* irq_routines[16] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0
};

void install_irq(int irq, void (*handler) (struct registers* r)) {
  irq_routines[irq] = handler;
}

void uninstall_irq(int irq) {
  irq_routines[irq] = 0;
}

/**
 * Give the two PICs the initialise command (code 0x11).
 * This command makes the PIC wait for 3 extra "initialisation words" on the data port.
 * These bytes give the PIC:
 * (1) Its vector offset. (ICW2)
 * (2) Tell it how it is wired to master/slaves. (ICW3)
 * (3) Gives additional information about the environment. (ICW4) 
 */
void irq_remap(void) {
  // Send init message to PICs.
  port_byte_out(PIC_MASTER_CMD_PORT, PIC_INIT_MSG);
  port_byte_out(PIC_SLAVE_CMD_PORT, PIC_INIT_MSG);

  // Send IDT offset (index) to PICs.:
  // 32 (0x20) for  master and 40 (0x28) for slave.
  port_byte_out(PIC_MASTER_DATA_PORT, PIC_MASTER_IDT_OFFSET);
  port_byte_out(PIC_SLAVE_DATA_PORT, PIC_SLAVE_IDT_OFFSET);

  // Send/Set Master-Slave relationship
  // - Let master know slave is at IRQ2:             00000100b (0x4)
  // - Let slave know it's "cascade identity" is 2:  00000010b (0x2)
  port_byte_out(PIC_MASTER_DATA_PORT, 0x04);
  port_byte_out(PIC_SLAVE_DATA_PORT, 0x02);

  // Give the PICs additional information
  port_byte_out(PIC_MASTER_DATA_PORT, 0x01);
  port_byte_out(PIC_SLAVE_DATA_PORT, 0x01);

  // Unmask interrupts?
  port_byte_out(PIC_MASTER_DATA_PORT, 0x0);
  port_byte_out(PIC_SLAVE_DATA_PORT, 0x0);
}

void install_irqs(void) {
  irq_remap();

  set_idt_entry(32, (unsigned) irq0, 0x08, 0x8E);   /* (idx=0,  desc=timer)    */
  set_idt_entry(33, (unsigned) irq1, 0x08, 0x8E);   /* (idx=1,  desc=keyboard) */
  set_idt_entry(34, (unsigned) irq2, 0x08, 0x8E);   /* (idx=2,  desc=unknown)  */
  set_idt_entry(35, (unsigned) irq3, 0x08, 0x8E);   /* (idx=3,  desc=unknown)  */
  set_idt_entry(36, (unsigned) irq4, 0x08, 0x8E);   /* (idx=4,  desc=serial port 1/3)  */
  set_idt_entry(37, (unsigned) irq5, 0x08, 0x8E);   /* (idx=5,  desc=unknown)  */
  set_idt_entry(38, (unsigned) irq6, 0x08, 0x8E);   /* (idx=6,  desc=unknown)  */
  set_idt_entry(39, (unsigned) irq7, 0x08, 0x8E);   /* (idx=7,  desc=unknown)  */
  set_idt_entry(40, (unsigned) irq8, 0x08, 0x8E);   /* (idx=8,  desc=unknown)  */
  set_idt_entry(41, (unsigned) irq9, 0x08, 0x8E);   /* (idx=9,  desc=unknown)  */
  set_idt_entry(42, (unsigned) irq10, 0x08, 0x8E);  /* (idx=10, desc=unknown)  */  
  set_idt_entry(43, (unsigned) irq11, 0x08, 0x8E);  /* (idx=11, desc=unknown)  */  
  set_idt_entry(44, (unsigned) irq12, 0x08, 0x8E);  /* (idx=12, desc=unknown)  */  
  set_idt_entry(45, (unsigned) irq13, 0x08, 0x8E);  /* (idx=13, desc=unknown)  */  
  set_idt_entry(46, (unsigned) irq14, 0x08, 0x8E);  /* (idx=14, desc=disk)     */  
  set_idt_entry(47, (unsigned) irq15, 0x08, 0x8E);  /* (idx=15, desc=unknown)  */  
}

// static char tmp[10] = "000000000";
void irq_handler(struct registers* r) {
 void (*handler) (struct registers* r);

  handler = irq_routines[r->int_no - 32];

  if (handler)
    handler(r);

  // If it's a slave interrupt, must send "complete" signal to slave too.
  if (r->int_no >= 40) {
    port_byte_out(PIC_SLAVE_CMD_PORT, 0x20);
  }

  port_byte_out(PIC_MASTER_CMD_PORT, 0x20);
}
