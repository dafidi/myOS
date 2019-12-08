#ifndef __FS_H__
#define __FS_H__

#include "fnode.h"
#include <kernel/error.h>

/**
 * WIP!
 */

struct master_fs_record {
  uint16_t magic_bits;
  lba_t fs_bitmap_first_block;
  lba_t fs_bitmap_last_block;
  struct folder_node fs_root_folder_node;
  uint32_t num_extra_master_fs_record_sectors;
}__attribute__((packed));

struct master_fs_record* master_record;
struct folder_node* root_folder_node;

enum sys_error init_fs(void);

void configure_pristine_fs(void);

#endif /* __FS_H__ */
