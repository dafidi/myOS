#ifndef __ISRS_H__
#define __ISRS_H__

#include "idt.h"
#include "system.h"

extern uint32_t isr0;
extern uint32_t isr1;
extern uint32_t isr2;
extern uint32_t isr3;
extern uint32_t isr4;
extern uint32_t isr5;
extern uint32_t isr6;
extern uint32_t isr7;
extern uint32_t isr8;
extern uint32_t isr9;
extern uint32_t isr10;
extern uint32_t isr11;
extern uint32_t isr12;
extern uint32_t isr13;
extern uint32_t isr14;
extern uint32_t isr15;
extern uint32_t isr16;
extern uint32_t isr17;
extern uint32_t isr18;
extern uint32_t isr19;
extern uint32_t isr20;
extern uint32_t isr21;
extern uint32_t isr22;
extern uint32_t isr23;
extern uint32_t isr24;
extern uint32_t isr25;
extern uint32_t isr26;
extern uint32_t isr27;
extern uint32_t isr28;
extern uint32_t isr29;
extern uint32_t isr30;
extern uint32_t isr31;

extern uint64_t isr64_0;
extern uint64_t isr64_1;
extern uint64_t isr64_2;
extern uint64_t isr64_3;
extern uint64_t isr64_4;
extern uint64_t isr64_5;
extern uint64_t isr64_6;
extern uint64_t isr64_7;
extern uint64_t isr64_8;
extern uint64_t isr64_9;
extern uint64_t isr64_10;
extern uint64_t isr64_11;
extern uint64_t isr64_12;
extern uint64_t isr64_13;
extern uint64_t isr64_14;
extern uint64_t isr64_15;
extern uint64_t isr64_16;
extern uint64_t isr64_17;
extern uint64_t isr64_18;
extern uint64_t isr64_19;
extern uint64_t isr64_20;
extern uint64_t isr64_21;
extern uint64_t isr64_22;
extern uint64_t isr64_23;
extern uint64_t isr64_24;
extern uint64_t isr64_25;
extern uint64_t isr64_26;
extern uint64_t isr64_27;
extern uint64_t isr64_28;
extern uint64_t isr64_29;
extern uint64_t isr64_30;
extern uint64_t isr64_31;

void install_isrs(void);
void install_isrs64(void);

#endif // __ISRS_H__
