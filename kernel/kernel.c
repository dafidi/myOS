// A simple kernel.
#include "../drivers/screen.h"
#include "idt.h"
#include "isrs.h"

void init(void) {
	init_idt();
	install_isrs();
}

void main() {
	char* hello_to_my_os_message = "Kernel loaded and now running!\n";
	print(hello_to_my_os_message);

	init();
	print("Kernel initialized.\n");
}
