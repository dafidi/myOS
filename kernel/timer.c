#include "timer.h"

#include "irq.h"
#include "low_level.h"
#include "system.h"

int timer_ticks = 0;

void init_timer(void) {
	timer_phase(DEFAULT_TIMER_FREQUENCY_HZ);
	timer_install();
}

void timer_phase(int hz) {
	int divisor = 1193180 / hz;
	port_byte_out(0x43, 0x36);
	port_byte_out(0x40, divisor & 0xff);
	port_byte_out(0x40, (divisor >> 8) );
}

void timer_handler(struct registers* r) {
	timer_ticks++;
}

void timer_wait(int ticks) {
	unsigned long eticks;
	eticks = timer_ticks + ticks;
	while(timer_ticks < eticks);
}

void timer_install(void) {
	install_irq(0, timer_handler);
}
