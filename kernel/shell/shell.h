#ifndef __SHELL_H__
#define __SHELL_H__

#define NUM_KNOWN_COMMANDS 4
#define SHELL_CMD_INPUT_LIMIT 1024

#include <kernel/system.h>

//  Exported variables used by keyboard driver.
char shell_scancode_buffer[SHELL_CMD_INPUT_LIMIT];
int shell_input_counter;

void exec_main_shell(void);

struct fs_context {
	struct dir_entry *curr_dir;
};

struct shell {
	struct fs_context fs_ctx;
	void (*exec) (void);
	void (*init) (void);
	void (*show_prompt) (void);
};
#endif
