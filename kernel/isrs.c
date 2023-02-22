#include "isrs.h"
#include "print.h"
#include "system.h"

#define __PAUSE_ON_FAULT__

#ifdef __PAUSE_ON_FAULT__
#define PAUSE_ON_FAULT() PAUSE()
#else
#define PAUSE_ON_FAULT()
#endif

char* exception_messages[] = {
        /*0 */ "Division By Zero Exception",
        /*1 */ "Debug Exception",
        /*2 */ "Non Maskable Interrupt Exception",
        /*3 */ "Breakpoint Exception",
        /*4 */ "Into Detected Overflow Exception",
        /*5 */ "Out of Bounds Exception",
        /*6 */ "Invalid Opcode Exception",
        /*7 */ "No Coprocessor Exception",
        /*8 */ "Double Fault Exception",
        /*9 */ "Coprocessor Segment Overrun Exception",
        /*10*/  "Bad TSS Exception",
        /*11*/  "Segment Not Present Exception",
        /*12*/  "Stack Fault Exception",
        /*13*/  "General Protection Fault Exception",
        /*14*/  "Page Fault Exception",
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

const char exception_message_part_1[] = "Fault: msg=[";
const char exception_message_part_2[] = "],code=[";
const char exception_message_part_3[] = "]\n";

void __install_isrs(void) {
    set_idt_entry(0,  addr_to_u32(&isr0),  0x08, 0x8E);
    set_idt_entry(1,  addr_to_u32(&isr1),  0x08, 0x8E);
    set_idt_entry(2,  addr_to_u32(&isr2),  0x08, 0x8E);
    set_idt_entry(3,  addr_to_u32(&isr3),  0x08, 0x8E);
    set_idt_entry(4,  addr_to_u32(&isr4),  0x08, 0x8E);
    set_idt_entry(5,  addr_to_u32(&isr5),  0x08, 0x8E);
    set_idt_entry(6,  addr_to_u32(&isr6),  0x08, 0x8E);
    set_idt_entry(7,  addr_to_u32(&isr7),  0x08, 0x8E);
    set_idt_entry(8,  addr_to_u32(&isr8),  0x08, 0x8E);
    set_idt_entry(9,  addr_to_u32(&isr9),  0x08, 0x8E);
    set_idt_entry(10, addr_to_u32(&isr10), 0x08, 0x8E);
    set_idt_entry(11, addr_to_u32(&isr11), 0x08, 0x8E);
    set_idt_entry(12, addr_to_u32(&isr12), 0x08, 0x8E);
    set_idt_entry(13, addr_to_u32(&isr13), 0x08, 0x8E);
    set_idt_entry(14, addr_to_u32(&isr14), 0x08, 0x8E);
    set_idt_entry(15, addr_to_u32(&isr15), 0x08, 0x8E);
    set_idt_entry(16, addr_to_u32(&isr16), 0x08, 0x8E);
    set_idt_entry(17, addr_to_u32(&isr17), 0x08, 0x8E);
    set_idt_entry(18, addr_to_u32(&isr18), 0x08, 0x8E);
    set_idt_entry(19, addr_to_u32(&isr19), 0x08, 0x8E);
    set_idt_entry(20, addr_to_u32(&isr20), 0x08, 0x8E);
    set_idt_entry(21, addr_to_u32(&isr21), 0x08, 0x8E);
    set_idt_entry(22, addr_to_u32(&isr22), 0x08, 0x8E);
    set_idt_entry(23, addr_to_u32(&isr23), 0x08, 0x8E);
    set_idt_entry(24, addr_to_u32(&isr24), 0x08, 0x8E);
    set_idt_entry(25, addr_to_u32(&isr25), 0x08, 0x8E);
    set_idt_entry(26, addr_to_u32(&isr26), 0x08, 0x8E);
    set_idt_entry(27, addr_to_u32(&isr27), 0x08, 0x8E);
    set_idt_entry(28, addr_to_u32(&isr28), 0x08, 0x8E);
    set_idt_entry(29, addr_to_u32(&isr29), 0x08, 0x8E);
    set_idt_entry(30, addr_to_u32(&isr30), 0x08, 0x8E);
    set_idt_entry(31, addr_to_u32(&isr31), 0x08, 0x8E);
}

void __install_isrs64(void) {
    set_idt64_entry(0,  addr_to_u64(&isr64_0), 0x08,  0x0, 0x8E);
    set_idt64_entry(1,  addr_to_u64(&isr64_1), 0x08,  0x0, 0x8E);
    set_idt64_entry(2,  addr_to_u64(&isr64_2), 0x08,  0x0, 0x8E);
    set_idt64_entry(3,  addr_to_u64(&isr64_3), 0x08,  0x0, 0x8E);
    set_idt64_entry(4,  addr_to_u64(&isr64_4), 0x08,  0x0, 0x8E);
    set_idt64_entry(5,  addr_to_u64(&isr64_5), 0x08,  0x0, 0x8E);
    set_idt64_entry(6,  addr_to_u64(&isr64_6), 0x08,  0x0, 0x8E);
    set_idt64_entry(7,  addr_to_u64(&isr64_7), 0x08,  0x0, 0x8E);
    set_idt64_entry(8,  addr_to_u64(&isr64_8), 0x08,  0x0, 0x8E);
    set_idt64_entry(9,  addr_to_u64(&isr64_9), 0x08,  0x0, 0x8E);
    set_idt64_entry(10, addr_to_u64(&isr64_10), 0x08, 0x0, 0x8E);
    set_idt64_entry(11, addr_to_u64(&isr64_11), 0x08, 0x0, 0x8E);
    set_idt64_entry(12, addr_to_u64(&isr64_12), 0x08, 0x0, 0x8E);
    // Use actual IST entries for GP and PF errors because we actually hit them.
    set_idt64_entry(13, addr_to_u64(&isr64_13), 0x08, 0x01, 0x8E);
    set_idt64_entry(14, addr_to_u64(&isr64_14), 0x08, 0x01, 0x8E);
    set_idt64_entry(15, addr_to_u64(&isr64_15), 0x08, 0x0, 0x8E);
    set_idt64_entry(16, addr_to_u64(&isr64_16), 0x08, 0x0, 0x8E);
    set_idt64_entry(17, addr_to_u64(&isr64_17), 0x08, 0x0, 0x8E);
    set_idt64_entry(18, addr_to_u64(&isr64_18), 0x08, 0x0, 0x8E);
    set_idt64_entry(19, addr_to_u64(&isr64_19), 0x08, 0x0, 0x8E);
    set_idt64_entry(20, addr_to_u64(&isr64_20), 0x08, 0x0, 0x8E);
    set_idt64_entry(21, addr_to_u64(&isr64_21), 0x08, 0x0, 0x8E);
    set_idt64_entry(22, addr_to_u64(&isr64_22), 0x08, 0x0, 0x8E);
    set_idt64_entry(23, addr_to_u64(&isr64_23), 0x08, 0x0, 0x8E);
    set_idt64_entry(24, addr_to_u64(&isr64_24), 0x08, 0x0, 0x8E);
    set_idt64_entry(25, addr_to_u64(&isr64_25), 0x08, 0x0, 0x8E);
    set_idt64_entry(26, addr_to_u64(&isr64_26), 0x08, 0x0, 0x8E);
    set_idt64_entry(27, addr_to_u64(&isr64_27), 0x08, 0x0, 0x8E);
    set_idt64_entry(28, addr_to_u64(&isr64_28), 0x08, 0x0, 0x8E);
    set_idt64_entry(29, addr_to_u64(&isr64_29), 0x08, 0x0, 0x8E);
    set_idt64_entry(30, addr_to_u64(&isr64_30), 0x08, 0x0, 0x8E);
    set_idt64_entry(31, addr_to_u64(&isr64_31), 0x08, 0x0, 0x8E);
}

/**
 * fault_handler - Handler for CPU exceptions.
 * 
 * @regs: Register values pushed to the stack.
 */
void fault_handler(struct registers* regs) {
    print_string(exception_message_part_1);
    print_string(exception_messages[regs->int_no]);
    print_string(exception_message_part_2);
    print_int32(regs->err_code);
    print_string(exception_message_part_3);

    print_registers(regs);

    PAUSE_ON_FAULT();
}

void fault_handler64(struct registers64* regs) {
    print_string(exception_message_part_1);
    print_string(exception_messages[regs->int_no]);
    print_string(exception_message_part_2);
    print_int32(regs->err_code);
    print_string(exception_message_part_3);

    print_registers64(regs);

    PAUSE_ON_FAULT();
}

void install_isrs(void) {
#ifdef CONFIG32
__install_isrs();
#else
__install_isrs64();
#endif
}
