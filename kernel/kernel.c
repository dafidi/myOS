// A simple kernel.
#include "system.h"

#include "idt.h"
#include "irq.h"
#include "isrs.h"
#include "mm.h"
#include "print.h"
#include "string.h"
#include "task.h"
#include "timer.h"

#include "shell/shell.h"

#include <drivers/disk/disk.h>
#include <drivers/keyboard/keyboard.h>
#include <fs/fs.h>

extern void enable_interrupts(void);
extern void initialize_idt(void);

static char* kernel_init_message = "Kernel initialized successfully.\n";
static char* kernel_load_message = "Kernel loaded and running.\n";

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

void *APP_START_VIRT_ADDR = (void *) 0x30000000;
void *APP_START_PHY_ADDR = (void *) 0x20000000;
int APP_SIZE = 28;
int APP_STACK_SIZE = 8192;
int APP_HEAP_SIZE = 16384;

void read_app_into_memory(void) {
	void *write_pos = APP_START_PHY_ADDR;
	int app_size = APP_SIZE;
	int amt_left = app_size;
	int amt_read = 0;

	while(amt_left) {
		int chunk_size = amt_left > 8192 ? 8192 : amt_left;
		int sector_offset = amt_read / 512 /* size of a sector. */;

		read_from_storage_disk(2 + sector_offset, chunk_size, write_pos);

		amt_left -= chunk_size;
		write_pos += chunk_size;
		amt_read += chunk_size;
	}
}

extern void fault_handler(struct registers *regs);
int main(void) {
	print_string(kernel_load_message);
	init();
	print_string(kernel_init_message);

	print_string("fault_handler is at "); print_int32(fault_handler); print_string("\n");

	read_app_into_memory();
	do_task_switch();

	exec_main_shell();

	while(true);

	return 0;
}
