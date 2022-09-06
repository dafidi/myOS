#ifndef __SHELL_H__
#define __SHELL_H__

#define SHELL_CMD_INPUT_LIMIT 1024

#include <kernel/system.h>
#include <fs/filesystem.h>

void exec_main_shell(void);

#endif
