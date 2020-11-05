#include "task.h"

extern unsigned int user_page_directory[USER_PAGE_TABLE_SIZE]__attribute__((aligned(0x1000)));
extern unsigned int kernel_page_directory[1024]__attribute__((aligned(0x1000)));

tss kernel_tss __attribute__((aligned(0x1000)));
tss user_tss __attribute__((aligned(0x1000)));

// Entry to dummy task that runs.
extern void dummy_branch(void);

void setup_tss(void) {
    tss *tss_ = &user_tss, *kernel_tss_ = &kernel_tss;

    tss_->previous_task_link_l16b = 5; // Shouldn't need to do this?
    tss_->CR3 = (unsigned int) user_page_directory;
    // TODO: Update these to use genuine user task values, not just copy kernel values.
    tss_->EIP = dummy_branch;
    __asm__("   movw %%es, %0 \n" : "=m" (tss_->ES_l16b) : );
    __asm__("   movw %%cs, %0 \n" : "=m" (tss_->CS_l16b) : );
    __asm__("   movw %%ss, %0 \n" : "=m" (tss_->SS_l16b) : );
    __asm__("   movw %%ds, %0 \n" : "=m" (tss_->DS_l16b) : );
    __asm__("   movw %%fs, %0 \n" : "=m" (tss_->FS_l16b) : );
    __asm__("   movw %%gs, %0 \n" : "=m" (tss_->GS_l16b) : );
    __asm__("   movw %%esp, %0 \n" : "=m" (tss_->ESP) : );
    __asm__("   movw %%ebp, %0 \n" : "=m" (tss_->EBP) : );

    kernel_tss_->CR3 = (unsigned int) kernel_page_directory;
    __asm__("   movw %%ss, %0 \n \
                movl %%esp, %1" : "=m" (kernel_tss_->SS_l16b), "=m" (kernel_tss_->ESP) : );
}

void do_task_switch(void) {
    print("attempting to loading task register.\n");
    load_kernel_tr();
    print("successfully loaded task register.\n");

    print("attempting task switch.\n");
    switch_task();
    print("task switch successful.\n");
}
