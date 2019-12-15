#ifndef __SHELL_H__
#define __SHELL_H__

#define NUM_KNOWN_COMMANDS 4
#define SHELL_CMD_INPUT_LIMIT 1024

char shell_scancode_buffer[SHELL_CMD_INPUT_LIMIT];
char shell_ascii_buffer[SHELL_CMD_INPUT_LIMIT];
int shell_input_counter;

void shell_exec(char* input);
void exec_known_cmd(int i);
void exec_main_shell(void);

void process_new_scancodes(int offset, int num_new_characters);
void process_cmd_input(void);
void show_prompt(void);

#endif
