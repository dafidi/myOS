#include "shell.h"

#include <kernel/system.h>
#include <kernel/string.h>
#include <drivers/screen/screen.h>
#include <drivers/keyboard/keyboard_map.h>
#include <fs/fs.h>
#include <fs/fnode.h>

const char stub[] = ">>";

char shell_ascii_buffer[SHELL_CMD_INPUT_LIMIT];
static char* known_commands[NUM_KNOWN_COMMANDS] = {
	"hi",
	"ls",
	"pwd",
	"addfile"
};
static int last_known_input_buffer_size = 0;

static void exec(char* input);
static void exec_known_cmd(int i);
void exec_main_shell(void);

static void process_new_scancodes(int offset, int num_new_characters);
static void process_cmd_input(void);

static void default_exec_routine(void);
static void default_show_prompt(void);
static void default_shell_init(void);

struct fs_context {
	uint32_t curr_folder_id;
};

struct shell {
	struct fs_context fs_ctx;
	void (*exec) (void);
	void (*init) (void);
	void (*show_prompt) (void);
};

static  struct folder_node* curr_folder_node;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// System-level input processing functions that can be used by any shell.
static void exec(char* input) {
	print("Attempting to execute: "); print(input); print("\n");
	char* known_cmd;
	int l, m;
	int i;

	for (known_cmd = known_commands[0], i = 0; i < NUM_KNOWN_COMMANDS; i++, known_cmd = known_commands[i]) {
		l = strlen(known_cmd);
		m = strlen(input);

		if (l != m) {
			continue;
		}

		if (strmatchn(known_cmd, input, l)) {
			exec_known_cmd(i);
			break;
		}
	}

	if (i == NUM_KNOWN_COMMANDS) {
		print("Sorry bud, can't help with that... yet!\n");
	}

}

static void exec_known_cmd(int i) {
	switch (i) {
		case 0:
			print("hi to you too!\n");
			break;
		case 1:
			print("ls? we're getting there dw!\n");
			break;
		case 2:
			print("pwd? don't worry, stay steady, we'll get there.\n");
			break;
		case 3:
			print("addfile? keep grinding.\n");
			break;
		default:
			print("doni't know what that is sorry :(\n");
	}
}

static void init_shell(struct shell* shell) {
	shell->init();
}

static void exec_shell(struct shell* shell) {
	shell->exec();
}

static char char_buff[2] = { '\0', '\0' };
static uint8_t scancode;
static char ascii_char;

static void process_new_scancodes(int offset, int num_new_characters) {
	int i;

	if (offset + num_new_characters >= SHELL_CMD_INPUT_LIMIT) {
		print("[Main Shell]:Input too large. Not processing\n");
		return;
	}

	for (i = 0; i < num_new_characters; i++) {
		 scancode = shell_scancode_buffer[offset + i];
		
		if (scancode & 0x80) {
			/* TODO: Handle key release actions. */
		} else if (scancode) {
			ascii_char = US_KEYBOARD_MAP[scancode];
			char_buff[0] =  ascii_char;
			char_buff[1] = 0;
			
			print(char_buff);

			if (ascii_char == '\n') {
				print("you entered: ["); print(shell_ascii_buffer); print("]\n");
				process_cmd_input();
				default_show_prompt();

				clear_buffer((uint8_t*)shell_ascii_buffer, SHELL_CMD_INPUT_LIMIT);
				clear_buffer((uint8_t*)shell_scancode_buffer, SHELL_CMD_INPUT_LIMIT);
				shell_input_counter = 0;
				last_known_input_buffer_size = 0;
				continue;
			} else {
				shell_ascii_buffer[offset + i] = ascii_char;
				// shell_input_counter++;
			}

			if (shell_input_counter >= SHELL_CMD_INPUT_LIMIT) {
				print("You have entered too many characters.  Resetting prompt...\n");
				clear_buffer((uint8_t*) shell_ascii_buffer, SHELL_CMD_INPUT_LIMIT);
				clear_buffer((uint8_t*) shell_scancode_buffer, SHELL_CMD_INPUT_LIMIT);        
				shell_input_counter = 0;
				last_known_input_buffer_size = 0;
				default_show_prompt();
			}
			
		}
	}
}

void process_cmd_input(void) {
	exec(shell_ascii_buffer);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Default/Main shell stuff.
static struct fs_context default_fs_ctx = { 
	.curr_folder_id = 0 
};

static struct shell default_shell = {
	.exec = default_exec_routine,
	.init = default_shell_init,
	.show_prompt = default_show_prompt
};

static void default_exec_routine(void) {
	shell_input_counter = 0;
	print("Main shell Executing.\n");
	default_show_prompt();
	while(true) {
		if (last_known_input_buffer_size < shell_input_counter) {
			process_new_scancodes(last_known_input_buffer_size,
													 shell_input_counter - last_known_input_buffer_size);
			last_known_input_buffer_size = shell_input_counter;
		} else if (last_known_input_buffer_size > shell_input_counter) {
			print("Something has gone terribly wrong with the shell. Exiting.\n");
			break;
		}
	}

	while(true);
}

static void default_shell_init(void) {
	default_shell.fs_ctx = default_fs_ctx;
}

static void default_show_prompt(void) {
	curr_folder_node = get_folder_node_by_id(
		default_shell.fs_ctx.curr_folder_id);

	print(curr_folder_node->name);
	print(stub);
}

// Executes the default/main shell.
void exec_main_shell(void) {
	init_shell(&default_shell);
	exec_shell(&default_shell);
}
