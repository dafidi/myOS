// A simple kernel.
#include "system.h"

#include "interrupts.h"
#include "mm.h"
#include "print.h"
#include "string.h"
#include "task.h"
#include "timer.h"

#include <drivers/disk/disk.h>
#include <drivers/keyboard/keyboard.h>
#include <fs/filesystem.h>

#include "shell/shell.h"

va_t APP_START_VIRT_ADDR = 0x30000000;
pa_t APP_START_PHY_ADDR = 0x20000000;
va_range_sz_t APP_STACK_SIZE = 8192;
va_range_sz_t APP_HEAP_SIZE = 16384;

static char *kernel_init_message = "Kernel initialized successfully.\n";
static char *kernel_load_message = "Kernel loaded and running.\n";

extern struct folder root_folder;

extern void system_test(void);

/**
 * init - Initialize system components.
 */
void init(void) {
    /* Set up fault handlers and interrupt handlers */
    /* but do not enable interrupts. */
    init_interrupts();

    /* Set up system's timer. */
    init_timer();

    /* Set up memory management. */
    init_mm();

    /* Set up task management. */
    init_task_system();

    /* Set up disk. */
    init_disk();

    /* Setup keyboard */
    init_keyboard();

    /* Set up fs. */
    init_fs();

    /* Let the fun begin, enable interrupts. */
    start_interrupts();
}

extern struct order_zone order_zones[];

/**
 * main - main kernel execution starting point.
 */
int main(void) {
    print_string(kernel_load_message);
    init();
    print_string(kernel_init_message);

    /*
    struct file file;
    int status;

    status = find_file(&root_folder, "app.bin", &file);
    if (status != 0) {
        print_string("file not found.");
        PAUSE();
    }

    execute_binary_file(&file);
    */
    system_test();
    exec_main_shell();

    // We should never reach here.
    INFINITE_LOOP();
}
