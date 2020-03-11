// A simple kernel.
#include "system.h"

#include "idt.h"
#include "isrs.h"
#include "irq.h"
#include "mm.h"
#include "timer.h"
#include "shell/shell.h"
#include "string.h"

#include <drivers/keyboard/keyboard.h>
#include <drivers/screen/screen.h>
#include <drivers/disk/disk.h>
#include <fs/fs.h>

extern void enable_interrupts(void);
extern void initialize_idt(void);

static char* kernel_load_message = "Kernel loaded and running.\n";
static char* kernel_init_message = "Kernel initialized successfully.\n";
static char* long_kernel_story =
	"============================================\n"
	"This is the story of a little kernel\n"
	"============================================\n";

void init(void) {
	/* Set up fault handlers and interrupt handlers. */
	init_idt();
	install_isrs();
	install_irqs();

	/* Set up system's timer. */
	timer_phase(DEFAULT_TIMER_FREQUENCY_HZ);
	timer_install();

	/* Set up memory management. */
	init_mm();

	/* Set up disk. */
	init_disk();

	/* Set up fs. */
	init_fs();

	/* Setup keyboard */
	install_keyboard();
	
	/* Let the fun begin. */
	enable_interrupts();
}

int main(void) {

	print(kernel_load_message);
	init();
	print(kernel_init_message);
	print(long_kernel_story);

	exec_main_shell();

	while(true);

	return 0;
}
