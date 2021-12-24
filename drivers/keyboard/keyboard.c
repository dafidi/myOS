#include "keyboard.h"

#include <kernel/irq.h>
#include <kernel/system.h>
#include <kernel/shell/shell.h>

extern char shell_scancode_buffer[SHELL_CMD_INPUT_LIMIT];
extern int shell_input_counter;

void keyboard_handler(struct registers* r) {
	int scancode;

	scancode = port_byte_in(KEYBOARD_DATA_REGISTER_PORT);
	if (!((uint8_t)scancode & 0x80)) {
		shell_scancode_buffer[shell_input_counter] = scancode;
		shell_input_counter++;
	}
}

void install_keyboard(void) {
	install_irq(1, keyboard_handler);
}

void init_keyboard(void) {
	install_keyboard();
}
