#include "fs.h"

#include <kernel/print.h>
#include <kernel/string.h>
#include <kernel/system.h>

#define NUM_SECTORS_ON_DISK 1<<21
#define FS_BITMAP_SIZE (NUM_SECTORS_ON_DISK) >> 3
#define F_OUT_BUFFER_SIZE 512
#define SECTOR_SIZE 512
#define NUM_DEFAULT_FOLDER_NODES 1

// Note about changing magic bits: x86 is little endian.
static const uint16_t magic_bits = 0xbaba;
static uint16_t master_record_buffer[512];
static uint8_t f_out_buffer[F_OUT_BUFFER_SIZE];
static int list_of_free_sectors[NUM_SECTORS_ON_DISK];

uint8_t fs_bitmap[FS_BITMAP_SIZE];

/**
 * Seems this function does work correctly but is extremely slow, especially when using a
 * bitmap of size 1 << 21 bytes.
*/
static int get_list_of_free_sectors() {
	int curr_sector_index = 0;
	int num_free_sectors = 0;
	uint8_t* ptr = fs_bitmap;
	uint8_t mini_bitmap;
	int nth_byte = 0;
	int i = 0;

	for (nth_byte = 0; nth_byte < FS_BITMAP_SIZE; nth_byte++) {

		mini_bitmap = *(ptr + nth_byte);
		for (i = 7; i >=0; i--) {
			
			if ((mini_bitmap >> i) & 0x1) {
				// Bit set, so sector is not free.
			} else {
				list_of_free_sectors[num_free_sectors] = curr_sector_index;
				num_free_sectors++;
			}
			curr_sector_index++;
		}
	}

	return num_free_sectors;
}

/**
 * init_fs
 * 
 * Perhaps a TODO: here is to check for and return errors.
 */
enum sys_error init_fs(void) {
	int num_free_sectors = 0;
	read_from_storage_disk(0, 512, master_record_buffer);
	master_record = (struct master_fs_record*) master_record_buffer;

	if (master_record->magic_bits == magic_bits) {
		print_string("Valid fs was found!\n");
		setup_fs_configs();
	} else {
		print_string("No Valid fs found! Configuring pristine fs.\n");
		clear_buffer(fs_bitmap, FS_BITMAP_SIZE);
		configure_pristine_fs();
	}

	// *(fs_bitmap) = 7;        // Just testing setting
	// *(fs_bitmap + 1) = 7;  // some bitmap bits.

	num_free_sectors = get_list_of_free_sectors();
	print_string("fs_bitmap is located at: ["); print_int32((int ) fs_bitmap); print_string("].\n");
	print_string("Size of the fs_bitmap is: ["); print_int32(sizeof(fs_bitmap)); print_string("].\n");
	print_string("The number of free sectors is ["); print_int32(num_free_sectors); print_string("]\n");
	return NONE;
}

void setup_fs_configs(void) {
	load_root_folder();

	int num_contents = root_folder_node->num_contents;
	print_string("There are "); print_int32(num_contents); print_string(" contents in the root folder.\n");
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
	set_bit(fs_bitmap, 0);
	// Clear f_out_buffer.
	clear_buffer(f_out_buffer, F_OUT_BUFFER_SIZE);

	// Write default_root_folder_node ro fs_out_buffer.
	write_folder_to_buffer(default_root_folder_node, f_out_buffer, F_OUT_BUFFER_SIZE);
	// Write f_out_buffer to sector 1.
	write_buffer_to_sector(f_out_buffer, F_OUT_BUFFER_SIZE, /*sector_number=*/1);
	// Clear f_out_buffer.
	clear_buffer(f_out_buffer, F_OUT_BUFFER_SIZE);
	set_bit(fs_bitmap, 1);

	// Write fs_bitmap to sector 2.
	write_fs_bitmap_to_disk(fs_bitmap, 2);

	*master_record = default_master_fs_record;
	*root_folder_node = default_root_folder_node;
}

void write_fs_bitmap_to_disk(uint8_t* bitmap, lba_t first_sector_number) {
	int num_sectors_needed;
	int n_bytes;
	int sector_idx; // Bitmap always starts from the 3rd sector. n=2.
	int c;                 // Count of sectors written to so far.

	num_sectors_needed = (FS_BITMAP_SIZE) / (SECTOR_SIZE);
	num_sectors_needed = num_sectors_needed == 0 ? 1 : num_sectors_needed;

	for (sector_idx=2, c=0; c < num_sectors_needed; c++, sector_idx++) {
		set_bit(fs_bitmap, sector_idx);
	}

	n_bytes = num_sectors_needed * SECTOR_SIZE;
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

// TODO (me)
void write_buffer_to_consecutive_sectors(const uint8_t* buffer, lba_t start_sector, int buffer_size) {
	// int curr_sector_offset = 0;
	int bytes_written = 0;

	while (bytes_written < buffer_size) {
		// if curr_sector_offset bit in bitmap is clear:
		//    write the next 512 bytes in buffer to start_sector + curr_sector_offset
		//
		//
		break;
	}
}

struct folder_node* check_and_get_from_child_nodes(struct folder_node* node, uint32_t id) {
	// WIP
	return root_folder_node;
}

struct folder_node* get_folder_node_by_id(uint32_t id) {
	struct folder_node* node = root_folder_node;

	if (node->id == id) {
		return node;
	}

	return check_and_get_from_child_nodes(node, id);
}
