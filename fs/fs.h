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

void setup_fs_configs(void);
void load_root_folder(void);
void construct_default_master_fs_record(void);
void construct_root_folder_node(void);
void configure_pristine_fs(void);
void write_fs_bitmap_to_disk(uint8_t* bitmap, lba_t first_sector_number);
void write_folder_to_buffer(struct folder_node folder, uint8_t* buffer, int buffer_size);
void write_buffer_to_sector(uint8_t* buffer, int buffer_size, int sector_number);
void write_master_record_to_buffer(struct master_fs_record record, uint8_t* buffer, int buffer_size);
void write_buffer_to_consecutive_sectors(const uint8_t* buffer, lba_t start_sector, int buffer_size);
void configure_pristine_fs(void);

#endif /* __FS_H__ */
