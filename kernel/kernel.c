// A simple kernel.
#include "system.h"

#include "interrupts.h"
#include "mm.h"
#include "print.h"
#include "string.h"
#include "task.h"
#include "timer.h"

#include <drivers/disk/disk.h>
#include <drivers/keyboard/keyboard.h>
#include <fs/fs.h>

#include "shell/shell.h"

va_t APP_START_VIRT_ADDR = 0x30000000;
pa_t APP_START_PHY_ADDR = 0x20000000;
va_range_sz_t APP_STACK_SIZE = 8192;
va_range_sz_t APP_HEAP_SIZE = 16384;
va_range_sz_t APP_SIZE = 28;

static char* kernel_init_message = "Kernel initialized successfully.\n";
static char* kernel_load_message = "Kernel loaded and running.\n";

/**
 * init - Initialize system components.
 */
void init(void) {
	/* Set up fault handlers and interrupt handlers */
	/* but do not enable interrupts. */
	init_interrupts();

	/* Set up system's timer. */
	init_timer();

	/* Set up disk. */
	init_disk();

	/* Set up memory management. */
	init_mm();

	/* Set up task management. */
	init_task_system();

	/* Set up fs. */
	init_fs();

	/* Setup keyboard */
	init_keyboard();

	/* Let the fun begin, enable interrupts. */
	start_interrupts();
}

/**
 * main - main kernel execution starting point.
 */
int main(void) {
	print_string(kernel_load_message);
	init();
	print_string(kernel_init_message);

	exec_waiting_tasks();

	exec_main_shell();

	while(true);

	return 0;
}
