// A simple kernel.
#include "system.h"

#include "idt.h"
#include "isrs.h"
#include "irq.h"
#include "timer.h"

#include <drivers/keyboard/keyboard.h>
#include <drivers/screen/screen.h>
#include <drivers/disk/disk.h>

extern void enable_interrupts(void);
extern void initialize_idt(void);

void init(void) {
	/* Set up fault handlers and interrupt handlers. */
	init_idt();
	install_isrs();
	install_irqs();

	/* Set up systems timer. */
	timer_phase(DEFAULT_TIMER_FREQUENCY_HZ);
	timer_install();

	/* Set up disk. */
	init_disk();

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

static char buffer1[1025] = "BUFFER 1 BEFORE";
static char buffer2[1025] = "BUFFER 2 BEFORE";

int main(void) {

	print(kernel_load_message);
	init();
	print(kernel_init_message);
	print(long_kernel_story);

	read_from_storage_disk(/*start_sector=*/0, /*n_bytes=*/1024, buffer1);
	read_from_storage_disk(1, 512, buffer2);

	while(true);
}
