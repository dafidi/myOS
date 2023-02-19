#include "task.h"

#include "low_level.h"
#include "print.h"

#include <drivers/disk/disk.h>

// If we stop using `--only-section=.text` to prepare binaries to run, we should
// figure out what the correct app start offset is.
#define APP_TASK_START_OFFSET 0x0

extern uint64_t _bss_end;

extern unsigned int user_page_directory[USER_PAGE_TABLE_SIZE];
extern unsigned int kernel_page_directory[1024];

extern struct gdt_entry pm_gdt[];
extern struct gdt_entry gdt64[];

extern va_t APP_START_VIRT_ADDR;
extern pa_t APP_START_PHY_ADDR;
extern va_range_sz_t APP_STACK_SIZE;
extern va_range_sz_t APP_HEAP_SIZE;
extern va_range_sz_t APP_SIZE;

/**
 * load_kernel_tr - This will load the CPU's task register using the descriptor
 * indexed by KERNEL_TASK_SEG_IDX.
 */
extern void load_kernel_tr(void);
/**
 * switch_to_user_task - This will trigger a task switch to the task described
 * by the USER_TASK descriptor in the GDT.
 */
extern void switch_to_user_task(void);

tss_t kernel_tss __attribute__((aligned(0x1000)));
tss_t user_tss __attribute__((aligned(0x1000)));
tss64_t kernel_tss64 __attribute__((aligned(0x1000)));

struct task_info dummy_task;

#ifdef CONFIG32
static void configure_kernel_tss(void) {
    tss_t *kernel_tss_ = &kernel_tss;

    kernel_tss_->CR3 = (unsigned int)(pa_t)kernel_page_directory;
}
#endif

uint64_t nearest_power_of_2(uint64_t val) {
    uint64_t power_of_2;
    int hidx, idx;

    // TODO: check if val is itself a power of two.

    idx = bit_scan_reverse64(val);
    hidx = idx / 4;

    power_of_2 = val >> (hidx * 4);
    power_of_2++;
    power_of_2 <<= (hidx * 4);

    return power_of_2;
}

#ifndef CONFIG32
static void configure_kernel_tss64(void) {
    tss64_t *kernel_tss_ = &kernel_tss64;

    // Place the interrupt stacks right after the _bss_end.
    uint64_t interrupt_stacks_begin = nearest_power_of_2((uint64_t)&_bss_end);

    __asm__("movq %%rsp, %0 \n" : "=m" (kernel_tss_->rsp0l) : );

    // If there are ever interrupt-related issues, it might be worth checking
    // whether any interrupt handler is using more than 4KiB (0x1000) of memory.
    __asm__("movq %%rax, %0 \n" : "=m" (kernel_tss_->ist1l) : "rax"(interrupt_stacks_begin + 0x1000));
    __asm__("movq %%rax, %0 \n" : "=m" (kernel_tss_->ist2l) : "rax"(interrupt_stacks_begin + 0x2000));
    __asm__("movq %%rax, %0 \n" : "=m" (kernel_tss_->ist3l) : "rax"(interrupt_stacks_begin + 0x3000));
    __asm__("movq %%rax, %0 \n" : "=m" (kernel_tss_->ist4l) : "rax"(interrupt_stacks_begin + 0x4000));
    __asm__("movq %%rax, %0 \n" : "=m" (kernel_tss_->ist5l) : "rax"(interrupt_stacks_begin + 0x5000));
    __asm__("movq %%rax, %0 \n" : "=m" (kernel_tss_->ist6l) : "rax"(interrupt_stacks_begin + 0x6000));
    __asm__("movq %%rax, %0 \n" : "=m" (kernel_tss_->ist7l) : "rax"(interrupt_stacks_begin + 0x7000));
}
#endif

/**
 * configure_user_tss - Configure hardware task structure from task_info.
 * Note: this is strictly for user-mode tasks.
 * 
 * @tss: hardware task structure.
 * @task: software task structure.
 */
void configure_user_tss(task_info* task) {
    const int data_seg = USER_DATA_SEGMENT_IDX;
    const int code_seg = USER_CODE_SEGMENT_IDX;
    tss_t *tss = &user_tss;

    // Select GDT segments for the program.
    tss->ES_l16b = (data_seg * 8) | 0x3;
    tss->CS_l16b = (code_seg * 8) | 0x3;
    tss->SS_l16b = (data_seg * 8) | 0x3;
    tss->DS_l16b = (data_seg * 8) | 0x3;
    tss->FS_l16b = (data_seg * 8) | 0x3;
    tss->GS_l16b = (data_seg * 8) | 0x3;

    // Set paging structure for program.
    tss->CR3 = (unsigned int)(pa_t)user_page_directory;

    // Set EIP, ESP and EBP registers for the program.
    tss->EIP = (unsigned int)task->start_virt_addr;
    tss->ESP = (unsigned int)task->start_virt_addr + task->mem_required + task->stack_size;
    tss->EBP = tss->ESP;
    
    // Presumably, only the kernel (ring 0) will run this code so it'll
    // probably(?) be okay to set the ring 0 stack pointer and segment
    // to the current ESP and SS registers.
    __asm__("   movl %%esp, %0 \n" : "=m" (tss->ESP0) : );
    __asm__("   mov %%ss, %0 \n" : "=m" (tss->SS0_l16b) : );
}


/**
 * load_task_into_ram - Read a task's program from disk into main memory.
 * 
 * @task: pointer to task to be loaded.
 * 
 * TODO: Handle disk load errors.
 */
void load_task_into_ram(task_info *task) {
    int start_sector = task->start_disk_sector;
    pa_t write_pos = task->start_phy_addr;
    int app_size = task->mem_required;
    int amt_left = app_size;
    int amt_read = 0;

    while(amt_left) {
        int chunk_size = amt_left > 8192 ? 8192 : amt_left;
        int sector_offset = amt_read / 512 /* size of a sector. */;

        read_from_storage_disk(start_sector + sector_offset, chunk_size, (void *)write_pos);

        amt_left -= chunk_size;
        write_pos += chunk_size;
        amt_read += chunk_size;
    }
}

/**
 * prepare_for_task_switch - set up virtual memory and track memory usage.
 * 
 * @task: task whose requirements are to be met.
 */
int prepare_for_task_switch(task_info *task) {
    va_range_sz_t requested_memory = task->mem_required + task->heap_size + task->stack_size;
    va_range_sz_t maybe_available_memory = (va_range_sz_t)get_available_memory();

    // Verify that there is enough free memory to load the app in.
    if (maybe_available_memory < requested_memory) {
        print_string("memory check failed: (requested, available) (");
        print_int32(requested_memory); print_string(","); print_int32(maybe_available_memory); print_string(")\n");
        return -1;
    }

    print_string("memory check passed: (requested, available) (");
    print_int32(requested_memory); print_string(","); print_int32(maybe_available_memory); print_string(")\n");
    
    if (reserve_and_map_user_memory(task->start_virt_addr, task->start_phy_addr, requested_memory) != 0)
        return -1;

    configure_user_tss(task);

    load_task_into_ram(task);

    return 0;
}

/**
 * clean_up_after_task - Here we just need to reclaim the task's memory and
 * clear the user_page_table_entries.
 * 
 * @task: the task which needs to be cleaned up.
 */
int clean_up_after_task(task_info *task) {
    va_range_sz_t requested_memory = task->mem_required + task->heap_size + task->stack_size;

    if (unreserve_and_unmap_user_memory(task->start_virt_addr, task->start_phy_addr, requested_memory))
        return -1;
    
    return 0;
}

/**
 * exec_waiting_tasks - In future, this will dequeue from a set of waiting
 * tasks and run them. For now, it just runs a dummy task :).
 * 
 * TODO: dynamic task waiting system.
 */
void exec_waiting_tasks(void) { }

void exec_task(struct task_info *task) {
    if (prepare_for_task_switch(task))
        return;

    print_string("attempting task switch.\n");
    switch_to_user_task();
    print_string("task switch successful.\n");

    clean_up_after_task(task);
}

void init_task_system(void) {
    // Set kernel and user TSS descriptors in GDT.
#ifdef CONFIG32
    make_gdt_entry(&pm_gdt[KERNEL_TSS_DESCRIPTOR_IDX], sizeof(kernel_tss), (unsigned int) &kernel_tss, 0x9, 0x18);
    make_gdt_entry(&pm_gdt[USER_TSS_DESCRIPTOR_IDX], sizeof(user_tss), (unsigned int) &user_tss, 0x9, 0x1e);
    configure_kernel_tss();
#else
    make_gdt64_tss_entry((struct gdt64_tss_entry *)&gdt64[KERNEL_TSS_DESCRIPTOR_IDX], sizeof(kernel_tss64), (uint64_t) &kernel_tss64, 0x9, 0x98);

    configure_kernel_tss64();
#endif

    load_kernel_tr();
}
