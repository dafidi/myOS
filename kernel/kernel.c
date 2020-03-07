// A simple kernel.
#include "system.h"

#include "idt.h"
#include "isrs.h"
#include "irq.h"
#include "timer.h"
#include "shell/shell.h"

#include <drivers/keyboard/keyboard.h>
#include <drivers/screen/screen.h>
#include <drivers/disk/disk.h>
#include <fs/fs.h>

extern unsigned int mem_map_buf_addr;
extern unsigned int mem_map_buf_entry_count;

extern void enable_interrupts(void);
extern void initialize_idt(void);

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

	/* Set up fs. */
	init_fs();

	/* Setup keyboard */
	install_keyboard();
	
	/* Let the fun begin. */
	enable_interrupts();
}

static char* kernel_load_message = "Kernel Loaded\n";
static char* kernel_init_message = "Kernel initialized.\n";
static char* long_kernel_story =
	"============================================\n"
	"This is the story of a little kernel\n"
	"============================================\n";

int main(void) {

	print("mem_map_buf_addr="); print_int32(mem_map_buf_addr);
	print("\n");
	print("mem_map_buf_entry_count="); print_int32(mem_map_buf_entry_count);
	print("\n");

	print(kernel_load_message);
	init();
	// set_screen_to_blue(); // just for fun hehe.
	print(kernel_init_message);
	print(long_kernel_story);

	exec_main_shell();

	while(true);

	return 0;
}
