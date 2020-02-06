#ifndef __SHELL_H__
#define __SHELL_H__

#define NUM_KNOWN_COMMANDS 4
#define SHELL_CMD_INPUT_LIMIT 1024

#include <kernel/system.h>
#include <drivers/screen/screen.h>

char shell_scancode_buffer[SHELL_CMD_INPUT_LIMIT];
char shell_ascii_buffer[SHELL_CMD_INPUT_LIMIT];
int shell_input_counter;
static int last_known_input_buffer_size = 0;

void shell_exec(char* input);
void exec_known_cmd(int i);
void exec_main_shell(void);

void process_new_scancodes(int offset, int num_new_characters);
void process_cmd_input(void);
void show_prompt(void);

struct fs_context {
  uint32_t curr_folder_id;
};

struct shell {
  struct fs_context fs_ctx;
  void (*exec) (void);
};

static struct fs_context default_fs_ctx = { 
  .curr_folder_id = 0 
};

static void default_exec_routine(void) {
  shell_input_counter = 0;

  show_prompt();

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

static struct shell default_shell = {
  .exec = default_exec_routine
};

#endif
