#include "task.h"

#include "print.h"

#include <drivers/disk/disk.h>

// If we stop using `--only-section=.text` to prepare binaries to run, we should
// figure out what the correct app start offset is.
#define APP_TASK_START_OFFSET 0x0

extern unsigned int user_page_directory[USER_PAGE_TABLE_SIZE];
extern unsigned int kernel_page_directory[1024];

extern struct gdt_entry pm_gdt[];

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

struct task_info dummy_task;

static void configure_kernel_tss(void) {
    tss_t *kernel_tss_ = &kernel_tss;

    kernel_tss_->CR3 = (unsigned int)kernel_page_directory;
}

/**
 * configure_user_tss - Configure hardware task structure from task_info.
 * Note: this is  strictly for user-mode tasks.
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
    tss->CR3 = (unsigned int)user_page_directory;

    // Set EIP, ESP and EBP registers for the program.
    tss->EIP = (unsigned int)task->start_virt_addr;
    tss->ESP = (unsigned int)task->start_virt_addr + task->mem_required + task->stack_size;
    tss->EBP = tss->ESP;
    
    // Presumably, only the kernel (ring 0) will run this code so it'll
    // probably(?) be okay to set the ring 0 stack pointer and segment
    // to the current ES and SS registers.
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
void exec_waiting_tasks(void) {
    // Setup task_info struct.
    // TODO: Make this dynamic, get rid of dummy_task.
    dummy_task.start_virt_addr = APP_START_VIRT_ADDR;
    dummy_task.start_phy_addr = APP_START_PHY_ADDR;
    dummy_task.mem_required = APP_SIZE;
    dummy_task.heap_size = APP_HEAP_SIZE;
    dummy_task.stack_size = APP_STACK_SIZE;
    dummy_task.start_disk_sector = 2;

    // Prepare for task switch. This involves verifying there's enough free
    // physical memory, allocating memory for the task, and configuring
    // virtual memory for the task.
    prepare_for_task_switch(&dummy_task);

    // If task switch preparation was okay, we can proceed to configure
    // hardware task params.
    configure_user_tss(&dummy_task);

    load_task_into_ram(&dummy_task);

    print_string("attempting task switch.\n");
    switch_to_user_task();
    print_string("task switch successful.\n");

    clean_up_after_task(&dummy_task);
}

void init_task_system(void) {
    // Set kernel and user TSS descriptors in GDT.
	make_gdt_entry(&pm_gdt[KERNEL_TSS_DESCRIPTOR_IDX], sizeof(kernel_tss), (unsigned int) &kernel_tss, 0x9, 0x18);
	make_gdt_entry(&pm_gdt[USER_TSS_DESCRIPTOR_IDX], sizeof(user_tss), (unsigned int) &user_tss, 0x9, 0x1e);

    configure_kernel_tss();

    load_kernel_tr();
}