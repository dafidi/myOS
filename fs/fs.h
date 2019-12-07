#ifndef __FS_H__
#define __FS_H__

#include <kernel/error.h>
#include "fnode.h"

/**
 * TBD!
 */

struct master_fs_record {
  struct file_node fs_bitmap_file_node;
  struct file_node fs_root_file_node;
  uint32_t num_extra_master_fs_record_sectors;
}__attribute__((packed));

struct master_fs_record* master_record;

enum sys_error init_fs(void);

#endif /* __FS_H__ */
