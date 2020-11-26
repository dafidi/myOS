// A simple kernel.
#include "system.h"

#include "idt.h"
#include "isrs.h"
#include "irq.h"
#include "mm.h"
#include "task.h"
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

	/* Set up disk. */
	init_disk();

	/* Set up memory management. */
	init_mm();

	/* Set up fs. */
	init_fs();

	/* Setup keyboard */
	install_keyboard();

	/* Let the fun begin. */
	enable_interrupts();
}

// Not yet used. Will use soon.
void *APP_PHY_ADDR = (void *) 0x2000000;
int main(void) {
	print(kernel_load_message);
	init();
	print(kernel_init_message);

	// Not yet used. Will use soon, app will load here.
	// TODO: Load and run app/binary.
	read_from_storage_disk(2, 512, APP_PHY_ADDR);

	do_task_switch();

	exec_main_shell();

	while(true);

	return 0;
}
