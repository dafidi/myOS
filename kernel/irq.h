#ifndef __IRQ_H__
#define __IRQ_H__

extern uint32_t irq0;
extern uint32_t irq1;
extern uint32_t irq2;
extern uint32_t irq3;
extern uint32_t irq4;
extern uint32_t irq5;
extern uint32_t irq6;
extern uint32_t irq7;
extern uint32_t irq8;
extern uint32_t irq9;
extern uint32_t irq10;
extern uint32_t irq11;
extern uint32_t irq12;
extern uint32_t irq13;
extern uint32_t irq14;
extern uint32_t irq15;

extern uint64_t irq64_0;
extern uint64_t irq64_1;
extern uint64_t irq64_2;
extern uint64_t irq64_3;
extern uint64_t irq64_4;
extern uint64_t irq64_5;
extern uint64_t irq64_6;
extern uint64_t irq64_7;
extern uint64_t irq64_8;
extern uint64_t irq64_9;
extern uint64_t irq64_10;
extern uint64_t irq64_11;
extern uint64_t irq64_12;
extern uint64_t irq64_13;
extern uint64_t irq64_14;
extern uint64_t irq64_15;

void install_irqs(void);
void install_irq(int,  void (*handler) (struct registers* r));

#endif /* __IRQ_H__ */
