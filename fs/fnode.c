#include <drivers/disk/disk.h>
#include <kernel/error.h>
#include <kernel/print.h>
#include <kernel/task.h>
#include <kernel/string.h>
#include <kernel/mm.h>

#include "fnode.h"

struct fs_master_record master_record;

struct fnode root_fnode;

struct dir_entry root_dir_entry = {
    .name = "/",
    .size = 0,
    .type = FOLDER,
};

/**
 * For now, we will not actually use these because some of them are so big,
 * we may not have enough memory for them. Instead when we need to access
 * them, we'll do so by accessing them on-disk, which is obviously very slow
 * but will suffice for now.
 */
uint8_t *fnode_table;
uint8_t *fnode_bitmap;
uint8_t *sector_bitmap;

struct mem_block *read_dir_content(const struct fnode *dir_fnode) {
    int buffer_size_power = 0, buffer_size = PAGE_SIZE, buffer_top = 0;
    int amt_read = 0, fnode_sector_idx = 0;
    struct mem_block *block;
    uint8_t *buffer;

    while (dir_fnode->size > buffer_size) {
        buffer_size_power += 1;
        buffer_size = 1 << (PAGE_SIZE_SHIFT + buffer_size_power);
    }

    block = zone_alloc(buffer_size_power);
    buffer = (uint8_t *)block->addr;

    while (amt_read < dir_fnode->size) {
        int bytes_to_read = ((dir_fnode->size - amt_read) >= SECTOR_SIZE) ? SECTOR_SIZE : (dir_fnode->size - amt_read);
        
        read_from_storage_disk(dir_fnode->sector_indexes[fnode_sector_idx], bytes_to_read, &buffer[buffer_top]);

        buffer_top += bytes_to_read;
        amt_read += bytes_to_read;
        fnode_sector_idx++;
    }

    return block;
}

void print_dir_entry(const struct dir_entry *dir_entry) {
    print_string("dir-entry: "); print_string(dir_entry->name); print_string("\n");
}

void show_dir_content(const struct fnode *dir_fnode) {
    struct dir_info *dir_info;
    uint8_t *dir_raw_content;
    struct mem_block *block;
    int num_entries;

    block = read_dir_content(dir_fnode);
    dir_raw_content = (uint8_t*) block->addr;
    dir_info = (struct dir_info*) dir_raw_content;

    struct dir_entry *dir_entry = dir_raw_content + sizeof(struct dir_info);
    for (num_entries = 0; num_entries < dir_info->num_entries; num_entries++, dir_entry++) {
        print_string("("); print_int32(num_entries); print_string(") "); print_dir_entry(dir_entry);
    }
    zone_free(block);
}

/**
 * @brief Set the nth bit on disk counting from the first bit in sector s.
 * 
 **/
void set_disk_bit(uint32_t n, uint32_t s) {
    int sector_offset = (n / 8/* bits_per_byte */) / SECTOR_SIZE;
    int bit_offset = n % (SECTOR_SIZE * 8/*bits_per_byte*/);
    uint8_t sector_buffer[SECTOR_SIZE];

    read_from_storage_disk(s + sector_offset, SECTOR_SIZE, sector_buffer);

    set_bit(sector_buffer, bit_offset);

    write_to_storage_disk(s + sector_offset, SECTOR_SIZE, sector_buffer);
}

/**
 * @brief set bit n in the fnode bitmap.
 *
 * TODO: when we're reading the structures into memory, this will be as simple as:
 * set_bit(fnode_bitmap, n) but for now, we have to work only on-disk.
 */
void fnode_bitmap_set(int n) {
    set_disk_bit(n, master_record.fnode_bitmap_start_sector);
}

/**
 * @brief set bit n in the sector bitmap.
 *
 * TODO: when we're reading the structures into memory, this will be as simple as:
 * set_bit(fnode_bitmap, n) but for now, we have to work only on disk.
 */
void sector_bitmap_set(int n) {
    set_disk_bit(n, master_record.sector_bitmap_start_sector);
}

void record_fnode_sector_bits(const struct fnode *_fnode) {
    int bytes_tracked = 0, sector_index = 0;

    while (bytes_tracked < _fnode->size) {
        bytes_tracked += ((_fnode->size - bytes_tracked) >= SECTOR_SIZE)
                        ? SECTOR_SIZE
                        : _fnode->size - bytes_tracked;
        sector_bitmap_set(_fnode->sector_indexes[sector_index++]);
    }
}

/**
 * @brief Recursively record in the fnode bitmap and sector bitmap the bits used by
 * the contents of a folder.
 * 
 * @param _fnode (MUST HAVE type = FOLDER)
 */
void __init_usage_bits(const struct fnode *_fnode) {
    struct dir_entry *dir_entry;
    struct dir_info *dir_info;
    uint8_t *dir_raw_content;
    struct mem_block *block;
    int num_entries;

    block = read_dir_content(_fnode);
    dir_raw_content = (uint8_t*) block->addr;
    dir_info = (struct dir_info*) dir_raw_content;

    dir_entry = dir_raw_content + sizeof(struct dir_info);
    for (num_entries = 0; num_entries < dir_info->num_entries; num_entries++, dir_entry++) {
        char sector_buffer[SECTOR_SIZE];
        struct fnode *__fnode;
        int sector_index = 0;

        // Mark the fnode used by this entry in the fnode bitmap.
        fnode_bitmap_set(dir_entry->fnode_location.fnode_table_index);
        
        // Read in the sector containing the fnode for this dir_entry.
        read_from_storage_disk(dir_entry->fnode_location.fnode_sector_index, SECTOR_SIZE, &sector_buffer);
        __fnode = &sector_buffer[dir_entry->fnode_location.fnode_sector_offset];

        // Mark the sectors occupied by this dir_entry's content.
        record_fnode_sector_bits(__fnode);

        // If it's a directory, recurse, so we can account for its children.
        if (dir_entry->type == FOLDER)
            __init_usage_bits(__fnode);
    }

    zone_free(block);
}

void init_fnode_bits(void) {
    // Record sectors used by the root_fnode content (dir_entrys).
    record_fnode_sector_bits(&root_fnode);

    // Mark in the fnode table the fnode used by the root fnode.
    fnode_bitmap_set(root_dir_entry.fnode_location.fnode_table_index);

    // Do similar for the root directory's children.
    __init_usage_bits(&root_fnode);
}

/**
 * @brief Initialize the fnode_bitmap and sector_bitmap.
 * 
 */
void init_usage_bits(void) {
    int num_bits;

    // Set the sector_bitmap bits occupied by master_record.
    sector_bitmap_set(0);
    print_string("m_r done,");

    // Set sector_bitmap bits occupied by fnode_bitmap.
    num_bits = master_record.fnode_bitmap_size / SECTOR_SIZE;
    for (int i = 0; i < num_bits; i++)
        sector_bitmap_set(master_record.fnode_bitmap_start_sector + i);
    print_string("f_b done,");

    // Set sector_bitmap bits occupieed by sector_bitmap.
    num_bits = master_record.sector_bitmap_size / SECTOR_SIZE;
    for (int i = 0; i < num_bits; i++)
        sector_bitmap_set(master_record.sector_bitmap_start_sector + i);
    print_string("s_b done,");

    // Set sector_bitmap bits occupied by fnode_table.
    num_bits = ((master_record.fnode_bitmap_size * 8) * sizeof(struct fnode)) / SECTOR_SIZE;
    for (int i = 0; i < num_bits; i++)
        sector_bitmap_set(master_record.fnode_table_start_sector + i);
    print_string("f_t done,");

    // Set fnode_bitmap bits occupied by actual files and folders.
    init_fnode_bits();
    print_string("d_b done\n");
}

void init_fs(void) {
    uint8_t sector_buffer[SECTOR_SIZE];
    char *root_dir_content;
    int num_entries = 0;

    // Read in the master record from disk.
    read_from_storage_disk(0, sizeof(struct fs_master_record), &master_record);
    root_dir_entry.fnode_location = master_record.root_dir_fnode_location;

    // Read root fnode in from disk.
    read_from_storage_disk(root_dir_entry.fnode_location.fnode_sector_index, SECTOR_SIZE, &sector_buffer);
    root_fnode = *((struct fnode *)&sector_buffer[root_dir_entry.fnode_location.fnode_sector_offset]);
    root_dir_entry.size = root_fnode.size;

    show_dir_content(&root_fnode);

    init_usage_bits();
}
