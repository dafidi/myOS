// A simple kernel.
#include "../drivers/screen.h"
#include "idt.h"
#include "isrs.h"
#include "irq.h"

extern void enable_interrupts(void);

void init(void) {
	init_idt();
	install_isrs();
	install_irqs();
	enable_interrupts();
}

void main() {
	char* kernel_load_message = "Kernel loaded and now running.\n";
	char* kernel_init_message = "Kernel initialized.\n";
	char* long_kernel_story = 
	"============================================\n"
	"This is the story of a little kernel\n"
	"============================================\n";
	print(kernel_load_message);
	init();
	// int i = 1;

	print(kernel_init_message);
	print(long_kernel_story);

	for (;;);
}
