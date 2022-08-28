#include "keyboard.h"

#include <kernel/irq.h>
#include <kernel/system.h>
#include <kernel/shell/shell.h>

extern char shell_scancode_buffer[SHELL_CMD_INPUT_LIMIT];
extern int shell_input_counter_;
extern int last_processed_pos_;
extern bool processing_input_;

int ignored_because_buffer_full_count_ = 0;

void keyboard_handler(struct registers* r) {
	uint8_t scancode;
	int head, tail;

	if (shell_input_counter_ - last_processed_pos_ == SHELL_CMD_INPUT_LIMIT) {
		ignored_because_buffer_full_count_++;
		return;
	}

	scancode = port_byte_in(KEYBOARD_DATA_REGISTER_PORT);

	shell_scancode_buffer[shell_input_counter_++ % SHELL_CMD_INPUT_LIMIT] = scancode;
}

void install_keyboard(void) {
	install_irq(1, keyboard_handler);
}

void init_keyboard(void) {
	install_keyboard();
}
