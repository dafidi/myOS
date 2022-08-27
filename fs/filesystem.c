#include <drivers/disk/disk.h>
#include <kernel/error.h>
#include <kernel/print.h>
#include <kernel/task.h>
#include <kernel/string.h>
#include <kernel/system.h>
#include <kernel/timer.h>
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

int add_dir_entry(struct fnode_location_t *dir_fnode_location, struct fnode *dir_fnode, struct dir_entry *new_entry) {
    uint8_t *sector_buffer, *sector_buffer_backup, *dir_info_buffer_backup;
    int last_sector_idx, used_in_last_sector, space_in_last_sector;
    int maybe_new_sector_index, error = 0;
    struct dir_info *dir_info;
    bool need_new_sector;

    // Might need to validate this trick but it seems like subtracting 1 here
    // helps avoid the case where fnode->size is a multiple of SECTOR_SIZE
    // causing last_sector_idx to be off-by-1 (+1).
    last_sector_idx = (dir_fnode->size - 1) / SECTOR_SIZE;
    used_in_last_sector = dir_fnode->size % SECTOR_SIZE ;
    used_in_last_sector = used_in_last_sector ? used_in_last_sector : SECTOR_SIZE;
    space_in_last_sector = ((last_sector_idx + 1) * SECTOR_SIZE) - dir_fnode->size;

    need_new_sector = space_in_last_sector < sizeof(struct dir_entry);

    // Allocate space large enough for 2 sectors in case there is not enough
    // space in the last sector to hold the data being added.
    sector_buffer = object_alloc(2 * SECTOR_SIZE);
    if (!sector_buffer) {
        print_string("Buffer alloc failed while add_dir_entry.\n");
        error = -1;
        goto exit;
    }
    sector_buffer_backup = object_alloc(2* SECTOR_SIZE);
    if (!sector_buffer_backup) {
        print_string("Buffer alloc for backup failed while add_dir_entry.\n");
        error = -1;
        goto exit;
    }
    clear_buffer(sector_buffer, 2 * SECTOR_SIZE);
    clear_buffer(sector_buffer_backup, 2 * SECTOR_SIZE);

    if (read_from_storage_disk(dir_fnode->sector_indexes[last_sector_idx], SECTOR_SIZE, sector_buffer)) {
        print_string("Failed to read in directory content");
        error = 1;
        goto exit_with_alloc;
    }
    // Backup the original dir content (i.e. dir_info and dir_entry) in case
    // we have a failure along the way.
    memory_copy(sector_buffer, sector_buffer_backup, SECTOR_SIZE);

    *((struct dir_entry *)&sector_buffer[used_in_last_sector]) = *new_entry;

    if (need_new_sector && query_free_sectors(1, &maybe_new_sector_index)) {
        print_string("Error getting new sector, can't add new dir_entry.\n");
        error = -1;
        goto exit_with_alloc;
    }

    dir_info = (struct dir_info*)sector_buffer;
    // If we are using fnode->sector_indexes[0], we know that dir_info lives
    // there so we can update dir_info.num_entries and avoid an additional
    // disk read/write to update dir_info.
    if (last_sector_idx == 0)
        dir_info->num_entries++;

    if (write_to_storage_disk(dir_fnode->sector_indexes[last_sector_idx], SECTOR_SIZE, sector_buffer)) {
        print_string("Failed to update last sector.\n");
        goto free_sector;
    } else if (need_new_sector) {
        if (write_to_storage_disk(maybe_new_sector_index, SECTOR_SIZE, sector_buffer + SECTOR_SIZE)) {
            print_string("Failed to write new sector.\n");
            error = -1;
            goto undo_last_sector_change;
        } else {
            dir_fnode->sector_indexes[last_sector_idx + 1] = maybe_new_sector_index;
        }
    }

    if (last_sector_idx == 0)
        goto size_inc;

    // The purpose of this read/write to disk is to update the fnode's dir_info
    // which is the first thing in the fnode's content, i.e. contained in
    // sector_indexes[0].
    if (read_from_storage_disk(dir_fnode->sector_indexes[0], SECTOR_SIZE, sector_buffer)) {
        print_string("Failed to read in directory content's 1st sector.\n");
        error = -1;
        goto undo_new_sector_change;
    }

    // Keep a backup of the dir_info sector in case we have to abandon the change(s).
    dir_info_buffer_backup = object_alloc(SECTOR_SIZE);
    memory_copy(sector_buffer, dir_info_buffer_backup, SECTOR_SIZE);

    dir_info->num_entries++;
    if (write_to_storage_disk(dir_fnode->sector_indexes[0], SECTOR_SIZE, sector_buffer)) {
        print_string("Failed to update dir_info sector.\n");
        error = -1;
        goto undo_new_sector_change;
    }

size_inc:
    dir_fnode->size += sizeof(struct dir_entry);
    if (save_fnode(dir_fnode_location, dir_fnode)) {
        if (last_sector_idx == 0)
            goto undo_new_sector_change;
        goto undo_dir_info_sector_change;
    }

    goto exit;

undo_dir_info_sector_change:
    dir_fnode->size -= sizeof(struct dir_entry);
    write_to_storage_disk(dir_fnode->sector_indexes[0], SECTOR_SIZE, dir_info_buffer_backup);
    object_free(dir_info_buffer_backup);

undo_new_sector_change:
    // When undoing the change to a new sector, we don't need to bother about
    // a write we may have done to the new sector. We only need to mark
    // the sector as free again, which we'll do in free_sector.
    dir_fnode->sector_indexes[last_sector_idx + 1] = 0;

undo_last_sector_change:
    write_to_storage_disk(dir_fnode->sector_indexes[last_sector_idx], SECTOR_SIZE, sector_buffer_backup);

free_sector:
    sector_bitmap_unset(maybe_new_sector_index, 1);

exit_with_alloc:
    object_free(sector_buffer);
    object_free(sector_buffer_backup);

exit:
    return error;
}

int save_file(struct fnode *fnode, struct new_file_info *file_info) {
    int error = 0, written = 0, to_write, sectors_written = 0;
    uint8_t *data = file_info->file_content;
    const int sz = file_info->file_size;

    // Write to disk one sector at a time because fnode->sector_indexes
    // might not be contiguous.
    while (written < sz) {
        int idx = fnode->sector_indexes[sectors_written];

        to_write = (sz - written) < SECTOR_SIZE ? (sz - written) : SECTOR_SIZE;
        if (write_to_storage_disk(idx, to_write, data)) {
            print_string("Failed to write file content to disk.\n");
            return -1;
        }

        sectors_written++;
        written += to_write;
        data += written;
    }

    return error;
}

int save_fnode(struct fnode_location_t *location, struct fnode *fnode) {
    uint8_t *sector_buffer;
    int error = 0;

    sector_buffer = object_alloc(SECTOR_SIZE);
    if (!sector_buffer)
       return -1;

    if (read_from_storage_disk(location->fnode_sector_index, SECTOR_SIZE, sector_buffer)) {
        print_string("Failed to read sector where new fnode should be written.\n");
        error = -1;
        goto exit_with_alloc;
    }

    *((struct fnode*)&sector_buffer[location->offset_within_sector]) = *fnode;

    if (write_to_storage_disk(location->fnode_sector_index, SECTOR_SIZE, sector_buffer)) {
        print_string("Failed to write sector where new fnode should be written.\n");
        error = -1;
    }

exit_with_alloc:
    object_free(sector_buffer);

    return error;
}

void unsave_fnode(struct fnode_location_t location) {
    // TODO.
}

int query_free_fnodes(int num_fnodes, struct fnode_location_t *fnode_indexes) {
    int visit_count = 0, free_count = 0, fnode_bitmap_current_sector_offset = 0;
    const int fnode_bitmap_start_sector = master_record.fnode_bitmap_start_sector;
    const int fnode_total = master_record.fnode_bitmap_size * BITS_PER_BYTE;
    const int fnodes_per_sector = SECTOR_SIZE / sizeof(struct fnode);
    const int block_size = ORDER_SIZE(5);
    const int bits_per_block = block_size * BITS_PER_BYTE;
    const int sector_skip = block_size / SECTOR_SIZE;
    struct mem_block *block = zone_alloc(block_size);
    uint8_t *block_buffer = (uint8_t*) block->addr;
    int error = 0;

    block = zone_alloc(block_size);
    if (!block) {
        print_string("Failed to allocate block while querying fnodes.\n");
        error = -1;
        goto exit_with_alloc;
    }
    block_buffer = (uint8_t*) block->addr;

    while (visit_count < fnode_total && free_count != num_fnodes) {
        int idx = fnode_bitmap_start_sector + fnode_bitmap_current_sector_offset;

        if (read_from_storage_disk(idx, block_size, block_buffer)) {
            print_string("Read failed in fnode search!\n");
            goto exit_reset_bitmap;
            return -1;
        }

        for (int i = 0; i < bits_per_block; i++, visit_count++) {
            int offset_within_sector, bit_offset, fnode_sector_index;

            if (get_bit(block_buffer, i))
                continue;

            bit_offset = fnode_bitmap_current_sector_offset * SECTOR_SIZE * BITS_PER_BYTE + i;
            fnode_sector_index = master_record.fnode_table_start_sector + bit_offset / fnodes_per_sector;
            offset_within_sector = (i % fnodes_per_sector) * sizeof(struct fnode);

            fnode_bitmap_set(bit_offset, 1);
            fnode_indexes[free_count++] = (struct fnode_location_t) {
                                            .fnode_table_index = bit_offset,
                                            .fnode_sector_index = fnode_sector_index,
                                            .offset_within_sector = offset_within_sector
                                          };

            if (free_count == num_fnodes)
                break;
        }

        fnode_bitmap_current_sector_offset += sector_skip;
    }

    if (free_count == num_fnodes)
        goto exit_with_alloc;

exit_reset_bitmap:
    print_string("Undoing changes to fnode_bitmap.\n");
    for (int i = 0; i < free_count; i++) {
        fnode_bitmap_unset(fnode_indexes[i].fnode_table_index, 1);
        fnode_indexes[i] = (struct fnode_location_t) { 0, 0, 0 };
    }

exit_with_alloc:
    zone_free(block);

    return error;
}

int query_free_sectors(int num_sectors, int *sector_indexes) {
    const int sector_bitmap_start_sector = master_record.sector_bitmap_start_sector;
    int visit_count = 0, free_count = 0, sector_bitmap_current_sector_offset = 0;
    const int sector_total = master_record.sector_bitmap_size * BITS_PER_BYTE;
    const int block_size = ORDER_SIZE(5);
    struct mem_block *block = zone_alloc(block_size);
    const int bits_per_block = block_size * BITS_PER_BYTE;
    const int sector_skip = block_size / SECTOR_SIZE;
    uint8_t *block_buffer;
    int error = 0;

    if (!block) {
        print_string("Unable to allocate block for sector query.\n");
        error = -1;
        goto exit_with_alloc;
    }
    block_buffer = (uint8_t *) block->addr;

    while (visit_count < sector_total && free_count != num_sectors) {
        int idx = sector_bitmap_start_sector + sector_bitmap_current_sector_offset;

        if (read_from_storage_disk(idx, block_size, block_buffer)) {
            print_string("Failed read in sector search!\n");
            error = -1;
            goto exit_bitmap_reset;
        }

        for (int i = 0; i < bits_per_block; i++, visit_count++) {
            int bit_index = sector_bitmap_current_sector_offset * SECTOR_SIZE * BITS_PER_BYTE + i;

            if (get_bit(block_buffer, i))
                continue;

            sector_bitmap_set(bit_index, 1);
            sector_indexes[free_count++] = bit_index;

            if (free_count == num_sectors)
                break;
        }

        sector_bitmap_current_sector_offset += sector_skip;
    }

    if (free_count == num_sectors)
        goto exit_with_alloc;

exit_bitmap_reset:
    print_string("Undoing changes to sector_bitmap.\n");
    for (int i = 0; i < free_count; i++) {
        sector_bitmap_unset(sector_indexes[i], 1);
        sector_indexes[i] = 0;
    }

exit_with_alloc:
    zone_free(block);

    return error;
}

int create_file(struct fs_context *ctx, struct new_file_info *file_info) {
    struct fnode_location_t ctx_location = ctx->curr_dir_fnode_location;
    struct fnode *parent_fnode = ctx->curr_dir_fnode;
    int sz = file_info->file_size, sz_sectors = 1;
    // TODO: We're allocating this on the stack because it'll probably be greater
    // than 2k which is that max dynamic object allocation allows, when we dynamic
    // allocation supports greater than 2k, we should use than instead of this
    // stack-allocation.
    int sector_indexes_buffer[MAX_FILE_CHUNKS];
    struct fnode_location_t new_fnode_location;
    struct dir_entry new_dir_entry;
    struct fnode new_fnode;
    int time, err;

    while ((sz_sectors << SECTOR_SIZE_SHIFT) < sz)
        sz_sectors++;

    if (sz_sectors > MAX_FILE_CHUNKS)
        return -1; // Unsupported file size.

    time_op(query_free_sectors(sz_sectors, sector_indexes_buffer), time, err);
    print_string("query_free_sectors took "); print_int32(time); print_string(" ticks.\n");
    if (err)
        return -1; // Not enough disk space.

    time_op(query_free_fnodes(1, &new_fnode_location), time, err);
    print_string("query_free_fnodes took "); print_int32(time); print_string(" ticks.\n");
    if (err)
        goto free_sectors; // Not enough fnodes.

    new_fnode.size = sz;
    new_fnode.type= FILE;
    for (int i = 0; i < sz_sectors; i++)
        new_fnode.sector_indexes[i] = sector_indexes_buffer[i];

    if (save_fnode(&new_fnode_location, &new_fnode))
        goto free_fnode;

    if (save_file(&new_fnode, file_info))
        goto unsave_new_fnode;

    memory_copy(file_info->name, new_dir_entry.name, MAX_FILENAME_LENGTH);
    new_dir_entry.size = sz;
    new_dir_entry.type = FILE;
    new_dir_entry.fnode_location = new_fnode_location;

    if (add_dir_entry(&ctx_location, parent_fnode, &new_dir_entry))
        goto unsave_new_fnode;

    return 0;

    // It's not really necessary to unsave a new fnode. We only need to mark the
    // bit free in the fnode bitmap.
unsave_new_fnode:
    unsave_fnode(new_fnode_location);

free_fnode:
    fnode_bitmap_unset(new_fnode_location.fnode_table_index, 1);

free_sectors:
    for (int i = 0; i < sz_sectors; i++)
        sector_bitmap_unset(sector_indexes_buffer[i], 1);

    return -1;
}

int read_dir_content(const struct fnode *dir_fnode, uint8_t *buffer) {
    int buffer_top = 0, amt_read = 0, fnode_sector_idx = 0;

    while (amt_read < dir_fnode->size) {
        int bytes_to_read = ((dir_fnode->size - amt_read) >= SECTOR_SIZE) ? SECTOR_SIZE : (dir_fnode->size - amt_read);
        int sector_idx = dir_fnode->sector_indexes[fnode_sector_idx++];

        if (read_from_storage_disk(sector_idx, bytes_to_read, &buffer[buffer_top]))
            return -1;

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
    if (read_dir_content(dir_fnode, buffer) < 0) {
        print_string("Unable to show dir content.\n");
        return;
    }

    dir_info = (struct dir_info*) buffer;
    dir_entry = (struct dir_entry *) (buffer + sizeof(struct dir_info));
    for (int i = 0; i < dir_info->num_entries; i++, dir_entry++) {
        print_string("("); print_int32(i); print_string(") "); print_dir_entry(dir_entry);
    }

    object_free(buffer);
}

int get_fnode(struct dir_entry *entry, struct fnode* fnode_ptr) {
    uint8_t buffer = object_alloc(SECTOR_SIZE);

    // Read fnode in from disk.
    if (read_from_storage_disk(entry->fnode_location.fnode_sector_index, sizeof(struct fnode), buffer))
        return -1;

    *fnode_ptr = (struct fnode)(*(struct fnode*)(buffer + entry->fnode_location.offset_within_sector));

    return 0;
}

int get_dir_info(struct fnode *fnode, struct dir_info *dir_info) {
    return read_from_storage_disk(fnode->sector_indexes[0], sizeof(struct dir_info), (uint8_t *) dir_info);
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

void fnode_bitmap_unset(uint32_t start_bit, uint64_t num_bits) {
    // TODO.
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

void sector_bitmap_unset(uint32_t start_bit, uint64_t num_bits) {
    // TODO.
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
        __fnode = &sector_buffer[dir_entry->fnode_location.offset_within_sector];

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
