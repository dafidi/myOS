#include <drivers/disk/disk.h>
#include <kernel/error.h>
#include <kernel/print.h>
#include <kernel/task.h>
#include <kernel/string.h>

#include "fnode.h"

extern va_t APP_START_VIRT_ADDR;
extern pa_t APP_START_PHY_ADDR;
extern va_range_sz_t APP_STACK_SIZE;
extern va_range_sz_t APP_HEAP_SIZE;
extern va_range_sz_t APP_SIZE;

// These are used where we need to dynamically provide file/folder.
// Ideally, we'd want dynamic memory allocation instead.
static struct file buffer_file;
static struct folder buffer_folder;

enum sys_error init_fs(void) {
    long long unsigned  disk_desc_region_size;
    long long unsigned disk_size;
    int master_record_block_idx;

    disk_size = 16ULL * GiB;
    disk_desc_region_size = (disk_size >> SECTOR_SIZE_SHIFT) << INT_SIZE_SHIFT;
    // The root folder begins immediately after the block descriptor region.
    master_record_block_idx = disk_desc_region_size >> SECTOR_SIZE_SHIFT;

    read_from_storage_disk(master_record_block_idx, sizeof(struct master_record), &__master_record);
    print_string("master_record's name is "); print_string(__master_record.name); print_string("\n");

    root_folder_desc = __master_record.root_folder_desc;
    read_from_storage_disk(root_folder_desc.block, root_folder_desc.len, &root_folder);
    // while(1);
    print_string("root_folder's name is "); print_string(root_folder.name); print_string("\n");
    print_string("root_folder's num_contents is "); print_int32(root_folder.num_contents); print_string("\n");

    return NONE;
}

int find_file(struct folder *folder, char *file_name, struct file *file) {
    int i = 0, n = strlen(file_name);

    for (i = 0; i < folder->num_contents; i++) {
        struct folder_content_desc *desc = &folder->content[i];

        if (desc->type == FILE) {
            read_from_storage_disk(desc->block, desc->len, &buffer_file);
            if (strmatchn(file_name, buffer_file.name, n)) {
                memory_copy((char *)&buffer_file, (char *)file, sizeof(struct file));
                return 0;
            }
        }
    }

    return -1;
}

int find_folder(struct folder *folder, char *folder_name, struct folder *target_folder) {
    int i = 0, n = strlen(folder_name);
    
    for (i = 0; i < folder->num_contents; i++) {
        struct folder_content_desc *desc = &folder->content[i];

        if (desc->type == FOLDER) {
            target_folder = &buffer_folder;

            read_from_storage_disk(desc->block, desc->len, &buffer_folder);
            if (strmatchn(folder_name, target_folder->name, n)) {
                *target_folder = buffer_folder;
                return 0;
            }
        }
    }

    return -1;
}

void execute_binary_file(struct file *file) {
    struct task_info task;

    task.start_virt_addr = APP_START_VIRT_ADDR;
    task.start_phy_addr = APP_START_PHY_ADDR;
    task.heap_size = APP_HEAP_SIZE;
    task.stack_size = APP_STACK_SIZE;
    // What do we do when a file has multiple chunks
    task.start_disk_sector = file->chunks[0].block;
    task.mem_required = file->chunks[0].len;
    
    exec_task(&task);
}
