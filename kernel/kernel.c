// A simple kernel.
#include "system.h"

#include "interrupts.h"
#include "mm/mm.h"
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

// Learnt this sorcery from this S/O question:
// https://stackoverflow.com/questions/59079951/getting-the-size-of-the-text-section-at-link-time-on-macos
// which pointed to https://github.com/apple-opensource-mirror/ld64/blob/9a22e880dba9ffe644cf5479d410d82a70c36b06/unit-tests/test-cases/segment-labels/main.c#L29
// where, apparently, there is a unit test for this exact thing.

#ifdef BUILDING_ON_MAC
extern int begin_text __asm("section$start$__TEXT$__text");
extern int end_text __asm("section$end$__TEXT$__text");
extern int begin_data __asm("section$start$__DATA$__data");
extern int end_data __asm("section$end$__DATA$__data");
extern int begin_bss __asm("section$start$__BSS$__bss");
extern int end_bss __asm("section$end$__BSS$__bss");

const uint64_t _text_start = (uint64_t)&begin_text;
const uint64_t _text_end = (uint64_t)&end_text;
const uint64_t _data_start = (uint64_t)&begin_data;
const uint64_t _data_end = (uint64_t)&end_data;
const uint64_t _bss_start = (uint64_t)&begin_bss;
const uint64_t _bss_end = (uint64_t)&end_bss;

#else

extern int begin_text;
extern int end_text;
extern int begin_data;
extern int end_data;
extern int begin_bss;
extern int end_bss;

const uint64_t _text_start = (uint64_t)&begin_text;
const uint64_t _text_end = (uint64_t)&end_text;
const uint64_t _data_start = (uint64_t)&begin_data;
const uint64_t _data_end = (uint64_t)&end_data;
const uint64_t _bss_start = (uint64_t)&begin_bss;
const uint64_t _bss_end = (uint64_t)&end_bss;

#endif // BUILDING_ON_MAC

extern struct folder root_folder;

extern void system_test(void);

/**
 * init - Initialize system components.
 */
void init(void) {
    /* Set up fault handlers and interrupt handlers */
    /* but do not enable interrupts.                */
    init_interrupts();

    /* Set up system's timer.                       */
    init_timer();

    /* Set up memory management.                    */
    init_mm();

    /* Set up task management.                      */
    /* This is done before here because this sets   */
    /* up the TSS which is necessary for interrupt  */
    /* handling in 64-bit mode.                     */
    init_task_system();

    /* Setup keyboard */
    init_keyboard();

    /* Let the fun begin, enable interrupts. This   */
    /* is done before fs because fs will need to    */
    /* read from disk, which will involve disk      */
    /* interrupts.                                  */
    start_interrupts();

    /* Set up disk. */
    init_disk();

    /* Set up fs. */
    init_fs();

    /* General system test. */
    system_test();
}

void bp() {

}
/**
 * main - main kernel execution starting point.
 */
int main(void) {
    // while(1);
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
    exec_main_shell();

    // We should never reach here.
    INFINITE_LOOP();
}
