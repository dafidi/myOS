#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include "kernel/system.h"
#include "kernel/low_level.h"

#define KEYBOARD_DATA_REGISTER_PORT 0x60

#define SHIFT_STATUS_INDEX 0
#define CTRL_STATUS_INDEX 1
#define ALT_STATUS_INDEX 2
#define CAPSLOCK_STATUS_INDEX 3
#define NUMLOCK_STATUS_INDEX 4
#define SCROLLOCK_STATUS_INDEX 5

void keyboard_handler(struct registers* r);

void install_keyboard(void);

#endif /* __KEYBOARD_H__ */
