#include "fs.h"
#include <drivers/screen/screen.h>
#include <kernel/system.h>
#include <kernel/string.h>

#define FS_BITMAP_SIZE 1<<18
#define F_OUT_BUFFER_SIZE 1024
#define NUM_DEFAULT_FOLDER_NODES 1

//Note about changing magic bits: x86 is little endian.
static const uint16_t magic_bits = 0xbaba;
static uint16_t master_record_buffer[1024];
static uint8_t f_out_buffer[F_OUT_BUFFER_SIZE];

uint8_t fs_bitmap[FS_BITMAP_SIZE];

char tmp[10] = "000000000";
enum sys_error init_fs(void) {
  read_from_storage_disk(0, 512, master_record_buffer);
  
  master_record = (struct master_fs_record*) master_record_buffer;

  if (master_record->magic_bits == magic_bits) {
    print("Valid fs was found!\n");
    setup_fs_configs();
  } else {
    print("No Valid fs found! Configuring pristine fs.\n");
    configure_pristine_fs();
  }

  return NONE;
}

void setup_fs_configs(void) {
  load_root_folder();

  int num_contents = root_folder_node->num_contents;
  int_to_string(tmp, num_contents, 9);
  print("there are "); print(tmp); print(" contents in the root folder.\n");
}

void load_root_folder(void) {
  // We do expect the master record is always at sector 0;
  *root_folder_node = master_record->fs_root_folder_node;
}

/*****************************************************************************/
// Stuff to generate pristine fs.
static struct master_fs_record default_master_fs_record;
static struct folder_node default_root_folder_node = { .name = "/" };

void construct_default_master_fs_record(void) {
  default_master_fs_record.magic_bits = 0xbaba;
  default_master_fs_record.fs_bitmap_first_block = 2;
  default_master_fs_record.fs_bitmap_last_block = 2 + ((FS_BITMAP_SIZE << 9) <= 0 ? 1 : (FS_BITMAP_SIZE << 9) ) - 1;
  default_master_fs_record.num_extra_master_fs_record_sectors = 0; // I don't really expect this to change, hopefully :) lol
  default_master_fs_record.fs_root_folder_node = default_root_folder_node;
}

void construct_root_folder_node(void) {
  default_root_folder_node.first_block = 1;
  default_root_folder_node.last_block = 1;
  default_root_folder_node.num_contents = 0;
  default_root_folder_node.id = 0;
  default_root_folder_node.folder_id_list = 0;
};

void configure_pristine_fs(void) {
  construct_default_master_fs_record();
  construct_root_folder_node();

  clear_buffer(f_out_buffer, F_OUT_BUFFER_SIZE);
  // Write master record to fs_out_buffer

  write_master_record_to_buffer(default_master_fs_record, f_out_buffer, F_OUT_BUFFER_SIZE);
  // Write f_out_buffer to sector 0
  write_buffer_to_sector(f_out_buffer, F_OUT_BUFFER_SIZE, /*sector_number=*/0);
  // Clear f_out_buffer.
  clear_buffer(f_out_buffer, F_OUT_BUFFER_SIZE);

  // Write default_root_folder_node ro fs_out_buffer.
  write_folder_to_buffer(default_root_folder_node, f_out_buffer, F_OUT_BUFFER_SIZE);
  // Write f_out_buffer to sector 1.
  write_buffer_to_sector(f_out_buffer, F_OUT_BUFFER_SIZE, /*sector_number=*/1);
  // Clear f_out_buffer.
  clear_buffer(f_out_buffer, F_OUT_BUFFER_SIZE);

  // Write fs_bitmap to sector 2.
  write_fs_bitmap_to_disk(fs_bitmap, FS_BITMAP_SIZE, 2);

  *master_record = default_master_fs_record;
  *root_folder_node = default_root_folder_node;
}

void write_fs_bitmap_to_disk(uint8_t* bitmap, int n_bytes, lba_t first_sector_number) {
  write_to_storage_disk(first_sector_number, n_bytes, bitmap);
}

void write_folder_to_buffer(struct folder_node folder, uint8_t* buffer, int buffer_size) {
  int record_size_bytes = sizeof(struct master_fs_record);
  int n_written = 0;
  char* ptr = (char*) &folder;

  for (int i = 0; i < record_size_bytes; i++) {
    buffer[i] = ptr[i];
    n_written++;
  }

  while (n_written < buffer_size) {
    buffer[n_written] = 0;
    n_written++;
  }
}

void write_buffer_to_sector(uint8_t* buffer, int buffer_size, int sector_number) {
  write_to_storage_disk(sector_number, buffer_size, buffer);
}

void write_master_record_to_buffer(struct master_fs_record record, uint8_t* buffer, int buffer_size) {
  int record_size_bytes = sizeof(struct master_fs_record);
  int n_written = 0;
  char* ptr = (char*) &record;

  for (int i = 0; i < record_size_bytes; i++) {
    buffer[i] = ptr[i];
    n_written++;
  }

  while (n_written < buffer_size) {
    buffer[n_written] = 0;
    n_written++;
  }
}

