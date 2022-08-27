#include "shell.h"

#include <drivers/keyboard/keyboard_map.h>
#include <drivers/disk/disk.h>
#include <fs/filesystem.h>
#include <kernel/mm.h>
#include <kernel/print.h>
#include <kernel/string.h>
#include <kernel/system.h>

extern struct fnode root_fnode;
extern struct dir_entry root_dir_entry;

extern void disk_test(void);

const char stub[3] = "$ ";
char shell_ascii_buffer[SHELL_CMD_INPUT_LIMIT];
static char* known_commands[NUM_KNOWN_COMMANDS] = {
	"hi",
	"ls",
	"pwd",
	"newf",
	"disk-test",
	"disk-id"
};
static int last_known_input_buffer_size = 0;

// Default/Main shell stuff.
static struct fs_context current_fs_ctx = {
		.curr_dir_fnode = &root_fnode,
};

static void default_exec_routine(void);
static void default_show_prompt(void);
static void default_shell_init(void);

struct shell default_shell = {
	.exec = default_exec_routine,
	.init = default_shell_init,
	.show_prompt = default_show_prompt
};

static void exec(char* input);
static void exec_known_cmd(int i);
void exec_main_shell(void);

static void process_new_scancodes(int offset, int num_new_characters);
static void process_cmd_input(void);

static void exec(char* input) {
	int i, l, m;

	print_string("Attempting to execute: "); print_string(input); print_string("\n");

	m = strlen(input);
	for (i = 0; i < NUM_KNOWN_COMMANDS; i++) {
		l = strlen(known_commands[i]);

		if (l != m)
			continue;

		if (strmatchn(known_commands[i], input, l))
			break;
	}

	if (i >= NUM_KNOWN_COMMANDS)
		print_string("Sorry, can't help with that... yet!\n");

	if (i < NUM_KNOWN_COMMANDS)
		exec_known_cmd(i);
}

static void exec_known_cmd(int i) {
	switch (i) {
		case 0:
			print_string("hi to you too!\n");
			break;
		case 1: {
			struct dir_info dir_info;

			if (get_dir_info(current_fs_ctx.curr_dir_fnode, &dir_info)) {
				print_string("Failed to get dir_info for [fnode=");
				print_int32(current_fs_ctx.curr_dir_fnode);
				print_string("]\n");
				return;
			}

			show_dir_content(current_fs_ctx.curr_dir_fnode);
			break;
		}
		case 2: {
			struct dir_info dir_info;

			if (get_dir_info(current_fs_ctx.curr_dir_fnode, &dir_info)) {
				print_string("Failed to get dir_info for [fnode=");
				print_int32(current_fs_ctx.curr_dir_fnode);
				print_string("]\n");
				return;
			}
			print_string("The current directory is: [");
			print_string(dir_info.name);
			print_string("].\n");
			break;
		}
		case 3: {
			char text[] = "Bien Venue!";
			struct new_file_info info = {
				.name = "new_file",
				.file_size = strlen(text),
				.file_content = text
			};

			print_string("One new file coming right up!\n");
			if (create_file(&current_fs_ctx, &info))
				print_string("Umm maybe next time. :( Losiento.\n");
			break;
		}
		case 4: {
			print_string("Running disk_test.\n");

			disk_test();
			break;
		}
		case 5: {
			print_string("Running \"IDENTIFY DEVICE\" ATA command.\n");
			identify_device();

			break;
		}
		default:
			print_string("don't know what that is sorry :(\n");
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
		print_string("[Main Shell]:Input too large. Not processing\n");
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

			print_string(char_buff);

			if (ascii_char == '\n') {
				print_string("you entered: ["); print_string(shell_ascii_buffer); print_string("]\n");
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
				print_string("You have entered too many characters.  Resetting prompt...\n");
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

static void default_show_prompt(void) {
	struct dir_info dir_info;

	clear_buffer(&dir_info, sizeof(struct dir_info));
	if (get_dir_info(current_fs_ctx.curr_dir_fnode, &dir_info)) {
		print_string("Failed to get dir_info for [fnode=");
		print_int32(current_fs_ctx.curr_dir_fnode);
		print_string("]\n");
		return;
	}

	print_string(dir_info.name);
	print_string(stub);
}

static void default_exec_routine(void) {
	shell_input_counter = 0;
	print_string("Main shell Executing.\n");
	default_show_prompt();
	while(true) {
		if (last_known_input_buffer_size < shell_input_counter) {
			process_new_scancodes(last_known_input_buffer_size,
								  shell_input_counter - last_known_input_buffer_size);
			last_known_input_buffer_size = shell_input_counter;
		} else if (last_known_input_buffer_size > shell_input_counter) {
			print_string("Something has gone terribly wrong with the shell. Exiting.\n");
			break;
		}
	}

	while(true);
}

static void default_shell_init(void) {
	current_fs_ctx.curr_dir_fnode_location = root_dir_entry.fnode_location;

	default_shell.fs_ctx = current_fs_ctx;
}

// Executes the default/main shell.
void exec_main_shell(void) {
	init_shell(&default_shell);
	exec_shell(&default_shell);
}
