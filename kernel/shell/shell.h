#ifndef __SHELL_H__
#define __SHELL_H__

#define NUM_KNOWN_COMMANDS 4
#define SHELL_CMD_INPUT_LIMIT 1024

#include <kernel/system.h>
#include <drivers/screen/screen.h>

//  Exported variables used by keyboard driver.
char shell_scancode_buffer[SHELL_CMD_INPUT_LIMIT];
int shell_input_counter;

 void exec_main_shell(void);
#endif
