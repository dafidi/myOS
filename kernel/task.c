#include "task.h"

#include "mm.h"
#include "print.h"

extern unsigned int user_page_directory[USER_PAGE_TABLE_SIZE]__attribute__((aligned(0x1000)));
extern unsigned int kernel_page_directory[1024]__attribute__((aligned(0x1000)));

extern void *APP_START_VIRT_ADDR;
extern void *APP_START_PHY_ADDR;
extern int APP_STACK_SIZE;
extern int APP_HEAP_SIZE;

extern void load_kernel_tr(void);
extern void switch_task(void);

tss kernel_tss __attribute__((aligned(0x1000)));
tss user_tss __attribute__((aligned(0x1000)));

#define APP_TASK_START_OFFSET 0x0

void setup_tss(void) {
    tss *tss_ = &user_tss, *kernel_tss_ = &kernel_tss;
    const int data_seg = 4, code_seg = 3;

    tss_->CR3 = (unsigned int) user_page_directory;

    // On inspection apps/app.bin appears to start the expected instructions
    // at an offset (when built without `--only-section=.text`).
    // Perhaps a TODO here is to figure out a general way of determining
    // where the task starts.
    // The other higher priority TODO is to support setting eip to the virtual
    // address. not the physical address.
    tss_->EIP = (unsigned int)APP_START_VIRT_ADDR + APP_TASK_START_OFFSET;

    tss_->ES_l16b = (data_seg * 8) | 0x3;
    tss_->CS_l16b = (code_seg * 8) | 0x3;
    tss_->SS_l16b = (data_seg * 8) | 0x3;
    tss_->DS_l16b = (data_seg * 8) | 0x3;
    tss_->FS_l16b = (data_seg * 8) | 0x3;
    tss_->GS_l16b = (data_seg * 8) | 0x3;
    tss_->ESP = (unsigned int)APP_START_VIRT_ADDR + APP_TASK_START_OFFSET + APP_STACK_SIZE;
    tss_->EBP = tss_->ESP;
    __asm__("   movl %%esp, %0 \n" : "=m" (tss_->ESP0) : );
    __asm__("   mov %%ss, %0 \n" : "=m" (tss_->SS0_l16b) : );

    kernel_tss_->CR3 = (unsigned int) kernel_page_directory;
}

void do_task_switch(void) {
    print_string("attempting to loading task register.\n");
    load_kernel_tr();
    print_string("successfully loaded task register.\n");

    print_string("attempting task switch.\n");
    switch_task();
    print_string("task switch successful.\n");
}
