#include <drivers/disk/disk.h>
#include <kernel/error.h>
#include <kernel/print.h>
#include <kernel/task.h>
#include <kernel/string.h>
#include <kernel/mm.h>

#include "filesystem.h"

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

int read_dir_content(const struct fnode *dir_fnode, uint8_t *buffer) {
    int buffer_top = 0, amt_read = 0, fnode_sector_idx = 0;

    while (amt_read < dir_fnode->size) {
        int bytes_to_read = ((dir_fnode->size - amt_read) >= SECTOR_SIZE) ? SECTOR_SIZE : (dir_fnode->size - amt_read);
        
        read_from_storage_disk(dir_fnode->sector_indexes[fnode_sector_idx++], bytes_to_read, &buffer[buffer_top]);

        buffer_top += bytes_to_read;
        amt_read += bytes_to_read;
    }

    return amt_read;
}

void print_dir_entry(const struct dir_entry *dir_entry) {
    print_string("dir-entry: "); print_string(dir_entry->name); print_string("\n");
}

void show_dir_content(const struct fnode *dir_fnode) {
    struct dir_entry *dir_entry;
    struct dir_info *dir_info;
    struct mem_block *block;
    uint8_t *buffer;

    buffer = object_alloc(dir_fnode->size);
    if (read_dir_content(dir_fnode, buffer) < 0 )
        return;

    dir_info = (struct dir_info*) buffer;
    dir_entry = (struct dir_entry *) (buffer + sizeof(struct dir_info));
    for (int i = 0; i < dir_info->num_entries; i++, dir_entry++) {
        print_string("("); print_int32(i); print_string(") "); print_dir_entry(dir_entry);
    }
    object_free(buffer);
}

int get_fnode(struct dir_entry *entry, struct fnode* fnode_ptr) {
    // Read fnode in from disk.
    return read_from_storage_disk(entry->fnode_location.fnode_sector_index, sizeof(struct fnode), (uint8_t *)fnode_ptr);
}

/**
 * @brief Set num_bits bits within a disk sector.
 * 
 * @param start_bit
 * @param num_bits
 * @param start_sector
 */
void set_sector_bits(uint32_t start_bit, uint64_t num_bits, uint32_t start_sector) {
    const int BITS_PER_SECTOR = 8 * SECTOR_SIZE;
    uint8_t sector_buffer[SECTOR_SIZE];
    int sector_offset, bit_offset;
    int bits_to_do = num_bits;

    sector_offset = start_bit / BITS_PER_SECTOR;
    bit_offset = start_bit % BITS_PER_SECTOR;

    while (bits_to_do) {
        int batchsize = ((bit_offset + bits_to_do) > BITS_PER_SECTOR)
                        ? BITS_PER_SECTOR - bit_offset
                        : bits_to_do;
        bits_to_do -= batchsize;

        read_from_storage_disk(start_sector + sector_offset, SECTOR_SIZE, sector_buffer);
        while (batchsize--)
            set_bit(sector_buffer, bit_offset++);
        write_to_storage_disk(start_sector + sector_offset++, SECTOR_SIZE, sector_buffer);

        // If we move on to the next sector, we want to start at the first (0th) bit in that sector.
        bit_offset = 0;
    }
}

/**
 * @brief set bit n in the fnode bitmap.
 *
 * TODO: when we're reading the structures into memory, this will be as simple as:
 * set_bit(fnode_bitmap, n) but for now, we have to work only on-disk.
 */
void fnode_bitmap_set(uint32_t start_bit, uint64_t num_bits) {
    set_sector_bits(start_bit, num_bits, master_record.fnode_bitmap_start_sector);
}

/**
 * @brief set bit n in the sector bitmap.
 *
 * TODO: when we're reading the structures into memory, this will be as simple as:
 * set_bit(fnode_bitmap, n) but for now, we have to work only on disk.
 */
void sector_bitmap_set(uint32_t start_bit, uint64_t num_bits) {
    set_sector_bits(start_bit, num_bits, master_record.sector_bitmap_start_sector);
}

void record_fnode_sector_bits(const struct fnode *_fnode) {
    int bytes_tracked = 0, sector_index = 0;

    while (bytes_tracked < _fnode->size) {
        bytes_tracked += ((_fnode->size - bytes_tracked) >= SECTOR_SIZE)
                        ? SECTOR_SIZE
                        : _fnode->size - bytes_tracked;
        sector_bitmap_set(_fnode->sector_indexes[sector_index++], 1);
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
    uint8_t *buffer;

    buffer = object_alloc(_fnode->size);
    read_dir_content(_fnode, buffer);

    dir_info = (struct dir_info*) buffer;
    dir_entry = buffer + sizeof(struct dir_info);
    for (int i = 0; i < dir_info->num_entries; i++, dir_entry++) {
        char sector_buffer[SECTOR_SIZE];
        struct fnode *__fnode;
        int sector_index = 0;

        // Mark the fnode used by this entry in the fnode bitmap.
        fnode_bitmap_set(dir_entry->fnode_location.fnode_table_index, 1);
        
        // Read in the sector containing the fnode for this dir_entry.
        read_from_storage_disk(dir_entry->fnode_location.fnode_sector_index, SECTOR_SIZE, &sector_buffer);
        __fnode = &sector_buffer[dir_entry->fnode_location.fnode_sector_offset];

        // Mark the sectors occupied by this dir_entry's content.
        record_fnode_sector_bits(__fnode);

        // If it's a directory, recurse, so we can account for its children.
        if (dir_entry->type == FOLDER)
            __init_usage_bits(__fnode);
    }

    object_free(buffer);
}

void init_fnode_bits(void) {
    // Record sectors used by the root_fnode content (dir_entrys).
    record_fnode_sector_bits(&root_fnode);

    // Mark in the fnode table the fnode used by the root fnode.
    fnode_bitmap_set(root_dir_entry.fnode_location.fnode_table_index, 1);

    // Do similar for the root directory's children.
    __init_usage_bits(&root_fnode);
}

/**
 * @brief Initialize the fnode_bitmap and sector_bitmap.
 * 
 */
void init_usage_bits(void) {
    uint32_t start_bit;
    uint64_t num_bits;

    // Set the sector_bitmap bits occupied by master_record.
    num_bits = 1;
    start_bit = 0;
    sector_bitmap_set(start_bit, num_bits);

    // Set sector_bitmap bits occupied by fnode_bitmap.
    num_bits = master_record.fnode_bitmap_size / SECTOR_SIZE;
    start_bit = master_record.fnode_bitmap_start_sector;
    sector_bitmap_set(start_bit, num_bits);

    // Set sector_bitmap bits occupieed by sector_bitmap.
    num_bits = master_record.sector_bitmap_size / SECTOR_SIZE;
    start_bit = master_record.sector_bitmap_start_sector;
    sector_bitmap_set(start_bit, num_bits);

    // Set sector_bitmap bits occupied by fnode_table.
    // Brief explanation of this calculation:
    //      - number_of_fnodes = bits_per_byte * fnode_bitmap_size_bytes (Since 1 bit -> 1 fnode,
    //          but ideally we should have a better way - this is something of a hack.)
    //      - size_of_fnode_table = number_of_fnodes  x  sizeof (struct fnode)
    //      - sectors_occupied by fnode_table = size_of_fnode_table / SECTOR_SIZE.
    num_bits = ((8 * (uint64_t) master_record.fnode_bitmap_size) * sizeof(struct fnode)) / SECTOR_SIZE;
    start_bit = master_record.fnode_table_start_sector;
    sector_bitmap_set(start_bit, num_bits);

    // Set fnode_bitmap bits occupied by actual files and folders.
    init_fnode_bits();
    print_string("d_b done\n");
}

void init_root_fnode(void) {
    root_dir_entry.fnode_location = master_record.root_dir_fnode_location;
    if (get_fnode(&root_dir_entry, &root_fnode)) {
        print_string("Error getting root_fnode.");
        return;
    }
    root_dir_entry.size = root_fnode.size;
}

void init_master_record(void) {
    read_from_storage_disk(0, sizeof(struct fs_master_record), &master_record);
}

void init_fs(void) {
    init_master_record();

    init_root_fnode();

    init_usage_bits();
}
