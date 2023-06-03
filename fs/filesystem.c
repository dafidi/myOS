#include <drivers/disk/disk.h>
#include <kernel/error.h>
#include <kernel/print.h>
#include <kernel/task.h>
#include <kernel/string.h>
#include <kernel/system.h>
#include <kernel/timer.h>
#include <kernel/mm/mm.h>

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

// On init, we set this to the max fnode ID seen among pre-existing files + 1.
uint32_t NEXT_FNODE_ID = 0;

int load_root_fnode(struct fnode *fnode) {
    if (get_fnode(&root_dir_entry, fnode))
        return -1;
    return 0;
}

int get_dir_name(struct fnode *fnode, char* name_buffer) {
    struct dir_info dir_info;
    int name_len = -1;

    if (get_dir_info(fnode, &dir_info)) {
        print_string("Error: Unable to get dir info during get name.\n");
        return -1;
    }

    name_len = strlen((char *) dir_info.name);
    memory_copy((char *) dir_info.name, name_buffer, name_len);
    return name_len;
}

/**
 * @brief Initialize a directory chain.
 *
 * Directory chains are initialized with an initial link whose id is
 * the root fnode's id.
 * This consumes some dynamic memory, so to avoid memory leaks, this
 * should generally be paired with destroy_directory_chain calls.
 *
 */
struct directory_chain *init_directory_chain(void) {
    struct directory_chain_link *chain_head;
    struct directory_chain *chain;
    struct fnode fnode;
    char *name_buffer;

    if (load_root_fnode(&fnode)) {
        print_string("Error: root fnode load failed during chain init.\n");
        return NULL;
    }

    chain_head = (struct directory_chain_link *) object_alloc(sizeof(struct directory_chain_link));
    if (!chain_head) {
       print_string("Error: unable to alloc chain_link. OOM?\n");
       return NULL;
    }

    name_buffer = (char *) object_alloc(MAX_FILENAME_LENGTH);
    if (!name_buffer) {
        print_string("Error: unable to allocate for name_buffer.\n");
        object_free((uint8_t *) chain_head);
        return NULL;
    }

    if (get_dir_name(&fnode, name_buffer) < 0) {
        print_string("Error: unable to get name for dir");
        object_free((uint8_t *) name_buffer);
        object_free((uint8_t *) chain_head);
        return NULL;
    }

    chain_head->next = NULL;
    chain_head->prev = NULL;
    chain_head->id = fnode.id;
    chain_head->name = name_buffer;

    chain = (struct directory_chain *) object_alloc(sizeof(struct directory_chain));
    if (!chain) {
        object_free((uint8_t *) chain_head);
        object_free((uint8_t *) name_buffer);
        print_string("Error: unable to alloc chain. OOM?\n");
        return NULL;
    }

    chain->head = chain_head;
    chain->tail = chain_head;
    chain->size = 1;
    return chain;
}

struct directory_chain *__init_directory_chain(struct directory_chain *orig) {
    struct directory_chain_link *origp;
    struct directory_chain *chain;

    chain = init_directory_chain();
    if (!chain) {
        print_string("Error: could not init chain. OOM? .\n");
        return NULL;
    }

    origp = orig->head;
    if (!origp) {
        print_string("Error: init_directory_chain: orig has no head.\n");
        goto fail_init;
    }

    // Skip past root folder link.
    origp = origp->next;

    while (origp) {
        struct fnode fnode;

        if (get_fnode_by_id(origp->id, &fnode)) {
            print_string("Error __init_directory_chain: getting fnode.\n");
            goto fail_init;
        }

        if (push_directory_chain_link(chain, &fnode)) {
            print_string("Error pushing link.\n");
            goto fail_init;
        }
        origp = origp->next;
    }

    return chain;

fail_init:
    destroy_directory_chain(chain);

    return NULL;
}

/**
 * @brief Deallocate all the memory used in a directory chain.
 *
 * @param chain
 */
void destroy_directory_chain(struct directory_chain *chain) {
    struct directory_chain_link *chainp = chain->head;

    object_free((uint8_t *) chain);

    if (!chainp)
        return;

    while (chainp->next) {
        object_free((uint8_t *) chainp->name);
        object_free((uint8_t *) chainp);
        chainp = chainp->next;
    }

    object_free((uint8_t *) chainp->name);
    object_free((uint8_t *) chainp);
}

/**
 * @brief Add a link to the end of a directory chain.
 *
 * @param chain
 * @param fnode;
 */
int push_directory_chain_link(struct directory_chain *chain, struct fnode *fnode) {
    struct directory_chain_link *chainp = chain->head;
    struct dir_info dir_info;
    fnode_id_t id = fnode->id;
    int error = 0;

    if (!chainp) {
        print_string("Error: pushing to NULL chain?\n");
        return -1;
    }

    while (chainp->next)
        chainp = chainp->next;

    if (chainp != chain->tail) {
        print_string("Something smells fishy in this chain.\n");
        return -1;
    }

    chainp->next = (struct directory_chain_link *)
                   object_alloc(sizeof(struct directory_chain_link));
    if (!chainp->next) {
        print_string("Error: alloc failed in chain push.\n");
        return -1;
    }

    chainp->next->id = id;
    chainp->next->next = NULL;
    chainp->next->prev = chainp;
    chain->tail = chainp->next;

    if (get_dir_info(fnode, &dir_info)) {
        print_string("Error getting dir info for push.\n");
        error = -1;
        goto remove_new_link;
    }

    chainp->next->name = (char *) object_alloc(strlen(dir_info.name) + 1);
    if (!chainp->next->name) {
        print_string("Error allocating object for name in chain push.\n");
        error = -1;
        goto remove_new_link;
    }
    memory_copy((char *)dir_info.name, chainp->next->name, strlen(dir_info.name));

    // There might be garbage left over in the allocated space, mark the
    // string's end.
    chainp->next->name[strlen(dir_info.name)] = '\0';

    chain->size++;

    return 0;

remove_new_link:
    chainp->next = NULL;
    chain->tail = chainp;
    object_free((uint8_t *) chainp->next);

    return error;
}

/**
 * @brief Remove a link from the end of a directory chain.
 *
 * @param chain
 */
int pop_directory_chain_link(struct directory_chain *chain) {
    struct directory_chain_link *chainp = chain->head;

    if (chain->size == 1) {
        print_string("Cannot pop lone (root) link, destroy the chain if you "
                     "must.\n");
        return -1;
    }

    if (!chainp) {
        print_string("Error: popping from NULL chain?\n");
        return -1;
    }

    while (chainp->next)
        chainp = chainp->next;

    if (chainp != chain->tail) {
        print_string("Error: pop detected a fishy chain.\n");
        return -1;
    }

    chainp = chainp->prev;
    object_free((uint8_t *) chainp->next->name);
    object_free((uint8_t *) chainp->next);
    chainp->next = NULL;

    chain->tail = chainp;
    chain->size--;

    return 0;
}

int validate_path_string(const char *path_str) {
    return 0;
}

/**
 * @brief Look for a file or folder named name within the
 * directory represented by dir_fnode.
 *
 * If found, put the fnode information of the file/folder in result_fnode.
 *
 * @param name - name of the file/folder being searched for.
 * @param dir_fnode - fnode of the folder where we should search for the
 *        file/folder.
 * @param result_fnode - if file/folder named name is found, this points to an fnode
 *        containing information about this file.
 */
int search_name_in_directory(char *name, struct fnode* dir_fnode, struct fnode *result_fnode) {
    uint8_t *buffer = object_alloc(dir_fnode->size);
    const int name_len = strlen(name);
    struct dir_entry *dir_entry;
    struct dir_info *dir_info;
    bool target_found = false;

    if (!buffer) {
        print_string("Error: alloc failed in name search.\n");
        return -1;
    }

    if (read_dir_content(dir_fnode, buffer) < 0) {
        print_string("Error during name search. read_dir_content_failed.\n");
        return -1;
    }

    dir_info = (struct dir_info *) buffer;
    dir_entry = (struct dir_entry *) (((char *) dir_info) + sizeof(struct dir_info));
    for (int i = 0; i < dir_info->num_entries; i++, dir_entry++) {
        const int entry_name_len = strlen(dir_entry->name);

        if (entry_name_len != name_len)
            continue;

        if (strmatchn(dir_entry->name, name, entry_name_len)) {
            get_fnode(dir_entry, result_fnode);
            target_found = true;
            break;
        }
    }
    object_free(buffer);

    if (target_found)
        return 0;

    return -1;
}

/**
 * @brief Check via the on-disk source of truth whether a
 * path (represented by the chain) is still valid.
 *
 * Fills in the fnode information for the innermost (tail) directory
 * and its location.
 */
int validate_directory_chain(struct directory_chain *chain, struct fnode *tail_dir_fnode, struct fnode_location_t *tail_dir_fnode_location) {
    struct fnode_location_t curr_fnode_location = root_dir_entry.fnode_location;
    struct directory_chain_link *chainp;
    bool chain_valid = false;
    struct fnode curr_fnode;

    if (get_fnode(&root_dir_entry, &curr_fnode)) {
        print_string("Error during chain vaildation: get_fnode failed.\n");
        return -1;
    }

    chainp = chain->head;
    if (chainp->id != curr_fnode.id) {
        print_string("Error: invalid chain, must start from root.\n");
        return -1;
    }

    chainp = chainp->next;
    if (!chainp)
        chain_valid = true; // A chain of just the root directory is valid.

    while (chainp) {
        uint8_t *buffer = object_alloc(curr_fnode.size);
        struct dir_entry *curr_dir_entry;
        struct dir_info *curr_dir_info;
        bool found_match = false;

        if (!buffer) {
            print_string("Error during chain validation: mem alloc failure.\n");
            return -1;
        }

        if (read_dir_content(&curr_fnode, buffer) < 0) {
            print_string("Error during chain validation: failed read_dir_content.\n");
            object_free(buffer);
            return -1;
        }

        curr_dir_info = (struct dir_info *) buffer;
        curr_dir_entry = (struct dir_entry *)
                         (((char *) curr_dir_info) + sizeof(struct dir_info));
        for (int i = 0; i < curr_dir_info->num_entries; i++, curr_dir_entry++) {
            struct fnode curr_entry_fnode;

            if (get_fnode(curr_dir_entry, &curr_entry_fnode)) {
                print_string("Error during chain validation: get_fnode failed.\n");
                object_free(buffer);
                return -1;
            }
            if (curr_entry_fnode.id == chainp->id) {
                curr_fnode = curr_entry_fnode;
                curr_fnode_location = curr_dir_entry->fnode_location;
                found_match = true;

                if (chainp->next == NULL)
                    chain_valid = true;
                break;
            }
        }

        object_free(buffer);
        if (!found_match)
            break;
        chainp = chainp->next;
    }

    if (chain_valid) {
        *tail_dir_fnode = curr_fnode;
        *tail_dir_fnode_location = curr_fnode_location;
    }

    return chain_valid ? 0 : -1;
}

/**
 * @brief Construct a struct directory_chain from a path string.
 *
 * This allocates a new directory_chain object. So, callers should take care to
 * pair with destroy appropriately to avoid memory leaks.
 *
 * @param ctx
 * @param path
 */
struct directory_chain *create_chain_from_path(struct fs_context *ctx, char *path) {
    struct fnode_location_t curr_dir_parent_fnode_location;
    struct fnode curr_dir_parent_fnode, curr_dir_fnode;
    char curr_dir_name[MAX_FILENAME_LENGTH + 1];
    int curr_dir_name_start_idx_in_path = 0;
    int curr_dir_name_end_idx_in_path = 0;
    const bool abs_path = path[0] == '/';
    struct directory_chain *chain;
    int curr_path_depth = 0;

    if (validate_path_string(path)) {
        print_string("Error creating chain: invalid path.\n");
        return NULL;
    }

    if (abs_path || !ctx || !ctx->working_directory_chain)
        chain = init_directory_chain();
    else
        chain = __init_directory_chain(ctx->working_directory_chain);

    if (validate_directory_chain(chain,
                                 &curr_dir_parent_fnode,
                                 &curr_dir_parent_fnode_location)) {
        print_string("Error: chain validation failed during chain creation.\n");
        destroy_directory_chain(chain);
        return NULL;
    }

    curr_path_depth += (int) abs_path;

next_name_idx_and_len:
    clear_buffer((uint8_t *) curr_dir_name, MAX_FILENAME_LENGTH + 1);

    while (path[curr_dir_name_start_idx_in_path] == '/' &&
           path[curr_dir_name_start_idx_in_path] != '\0')
        curr_dir_name_start_idx_in_path++;

    curr_dir_name_end_idx_in_path = curr_dir_name_start_idx_in_path;
    while (path[curr_dir_name_end_idx_in_path] != '/' &&
           path[curr_dir_name_end_idx_in_path] != '\0')
        curr_dir_name_end_idx_in_path++;

    if (curr_dir_name_start_idx_in_path == curr_dir_name_end_idx_in_path)
        goto done;

    memory_copy((char *)&path[curr_dir_name_start_idx_in_path],
                curr_dir_name,
                curr_dir_name_end_idx_in_path - curr_dir_name_start_idx_in_path);

    if (strlen(curr_dir_name) == 2 && strmatchn(curr_dir_name, "..", strlen(curr_dir_name))) {
        if (chain->size == 1) {
            print_string("Error: path resolves to (nonexistent) parent of root dir.\n");
            goto revert_chain_changes;
        }

        pop_directory_chain_link(chain);
        curr_path_depth--;

        // We don't really need to validate here, we just want the fnode
        // of the grandparent directory and this is currently the simplest way
        // to get it.
        // TODO: Create a function that's a simpler way to get the grandparent
        // fnode.
        if (validate_directory_chain(chain,
                                     &curr_dir_parent_fnode,
                                     &curr_dir_parent_fnode_location)) {
             print_string("Error: although this validation isn't needed, "
                          "there's no reason for it to fail.\n");
             goto revert_chain_changes;
        }
        curr_dir_name_start_idx_in_path = curr_dir_name_end_idx_in_path;
        goto next_name_idx_and_len;
    }

    if (search_name_in_directory(curr_dir_name, &curr_dir_parent_fnode, &curr_dir_fnode)) {
        print_string("Error creating chain: unfound name in path.\n");
        goto revert_chain_changes;
    }

    if (push_directory_chain_link(chain, &curr_dir_fnode)) {
        print_string("Error: link push failed during create chain.\n");
        goto revert_chain_changes;
    }

    curr_dir_name_start_idx_in_path = curr_dir_name_end_idx_in_path;
    curr_dir_parent_fnode = curr_dir_fnode;
    curr_path_depth++;

    goto next_name_idx_and_len;

done:
    return chain;

revert_chain_changes:
    if (abs_path)
        destroy_directory_chain(chain);
    else
        // TODO: curr_path_depth may actually be negative, so handle this.
        while (curr_path_depth--)
            pop_directory_chain_link(chain);

    return NULL;
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
    int bits_to_do = num_bits, bit_offset;
    uint8_t sector_buffer[SECTOR_SIZE];
    int sector_offset;

    sector_offset = start_bit / BITS_PER_SECTOR;
    bit_offset = start_bit % BITS_PER_SECTOR;

    while (bits_to_do) {
        int batchsize = ((bit_offset + bits_to_do) > BITS_PER_SECTOR)
                        ? BITS_PER_SECTOR - bit_offset
                        : bits_to_do;
        bits_to_do -= batchsize;

        read_from_storage_disk(start_sector + sector_offset, SECTOR_SIZE, sector_buffer);
        while (batchsize--)
            set_bit((uint8_t*)sector_buffer, bit_offset++);
        write_to_storage_disk(start_sector + sector_offset++, SECTOR_SIZE, sector_buffer);

        // If we move on to the next sector, we want to start at the first (0th) bit in that sector.
        bit_offset = 0;
    }
}

/**
 * @brief Unset num_bit bits on disk starting from start_sector);
 *
 * @param start_bit
 * @param num_bits
 * @param start_sector
 */
void unset_sector_bits(uint32_t start_bit, uint64_t num_bits, uint32_t start_sector) {
    const int BITS_PER_SECTOR = BITS_PER_BYTE * SECTOR_SIZE;
    int sector_offset = start_bit / BITS_PER_SECTOR;
    int bit_offset = start_bit % BITS_PER_SECTOR;
    uint8_t sector_buffer[SECTOR_SIZE];
    int sector;

    while (num_bits) {
        sector = start_sector + sector_offset;

        read_from_storage_disk(sector, SECTOR_SIZE, sector_buffer);
        while (num_bits && bit_offset < BITS_PER_SECTOR) {
            clear_bit(sector_buffer, bit_offset++);
            num_bits--;
        }
        write_to_storage_disk(sector, SECTOR_SIZE, sector_buffer);

        bit_offset = 0;
        sector_offset++;
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
    unset_sector_bits(start_bit, num_bits, master_record.fnode_bitmap_start_sector);
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
    unset_sector_bits(start_bit, num_bits, master_record.sector_bitmap_start_sector);
}

/**
 * @brief List out the files and folders in the innermost directory
 * in the given directory_chain.
 *
 * @param chain - directory path representation.
 * @param path
 */
int list_dir_content(struct fs_context *ctx, char *path) {
    struct fnode_location_t tail_dir_fnode_location;
    struct directory_chain *new_chain = NULL;
    struct fnode tail_dir_fnode;

    if (path) {
        new_chain = create_chain_from_path(ctx, path);
        if (!new_chain) {
            print_string("Error list_dir_content: create_chain_from_path.\n");
            return -1;
        }
    } else {
        new_chain = ctx->working_directory_chain;
    }

    if (validate_directory_chain(new_chain, &tail_dir_fnode, &tail_dir_fnode_location)) {
        print_string("Error: dir show failed on chain validation.\n");
        return -1;
    }

    show_dir_content(&tail_dir_fnode);

    if (path)
        destroy_directory_chain(new_chain);

    return 0;
}

/**
 * @brief Search for a file/folder in the innermost folder of a directory_chain.
 *
 * Although the logic seems to apply to both files and folders, the description
 * is targeted at folders because this function was initially written for the
 * purpose of handling a change-directory command where we would be looking for
 * target directory in the chain'd innermost directory.
 *
 * @param chain
 * @param target_dir_name
 * @param result_fnode
 */
int fs_search(struct directory_chain *chain, char* target_dir_name, struct fnode *result_fnode) {
    struct fnode_location_t tail_dir_fnode_location;
    struct fnode tail_dir_fnode;

    if (validate_directory_chain(chain, &tail_dir_fnode, &tail_dir_fnode_location)) {
        print_string("Error: Invalid directory_chain.\n");
        return -1;
    }

    if (search_name_in_directory(target_dir_name, &tail_dir_fnode, result_fnode)) {
        print_string("Error: could not find "); print_string(target_dir_name); print_string(" in chain provided.\n");
        return -1;
    }

    return 0;
}

/** @brief Search for num_fnodes free fnodes.
 *
 * This amounts to looking for the num_fnodes unset bits within the fnode bitmap
 * and is made complicated by the fact that the fnode_bitmap (for now) resides on
 * disk.
 *
 * @param num_fnodes
 * @Param fnode_indexes an output array of the indexes of the free fnodes found.
 */
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

/**
 * @brief Search for unused sectors.
 *
 * Find num_sectors unused sectors. This amounts to a search for num_sectors unset bits
 * in the sector_bitmap and is complicated by the fact sector_bitmap is on-disk.
 *
 * @param num_sectors
 * @param sector_indexes output array of indexes of free sectors.
 */
int query_free_sectors(int num_sectors, int *sector_indexes) {
    int bitmap_sector = (master_record.data_blocks_start_sector / BITS_PER_BYTE) >> SECTOR_SIZE_SHIFT;
    const int sector_total = master_record.sector_bitmap_size * BITS_PER_BYTE;
    const int block_size = ORDER_SIZE(5);
    struct mem_block *block = zone_alloc(block_size);
    const int bits_per_block = block_size * BITS_PER_BYTE;
    const int sector_skip = block_size / SECTOR_SIZE;
    int error = 0, visit_count = 0, free_count = 0;
    uint8_t *block_buffer;

    if (!block) {
        print_string("Unable to allocate block for sector query.\n");
        error = -1;
        goto exit_with_alloc;
    }
    block_buffer = (uint8_t *) block->addr;

    while (visit_count < sector_total && free_count != num_sectors) {
        int idx = master_record.sector_bitmap_start_sector + bitmap_sector;

        if (read_from_storage_disk(idx, block_size, block_buffer)) {
            print_string("Failed read in sector search!\n");
            error = -1;
            goto exit_bitmap_reset;
        }

        for (int i = 0; i < bits_per_block; i++, visit_count++) {
            int bit_index = bitmap_sector * SECTOR_SIZE * BITS_PER_BYTE + i;

            if (get_bit(block_buffer, i))
                continue;

            sector_bitmap_set(bit_index, 1);
            sector_indexes[free_count++] = bit_index;

            if (free_count == num_sectors)
                break;
        }

        bitmap_sector += sector_skip;
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

/**
 * @brief Save the contents of the firl described by file_info to disk.
 *
 * @param fnode fnode of the file to be saved to disk.
 * @param file_info
 */
int save_file(struct fnode *fnode, struct file_creation_info *file_info) {
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

/**
 * @brief Save the content of the folder described by fnode to disk.
 *
 * @param fnode
 * @param folder_info
 */
int save_folder(struct fnode *fnode, struct folder_creation_info *folder_info) {
    int error = 0, written = 0, to_write, sectors_written = 0;
    const int sz = folder_info->size;
    uint8_t *data = folder_info->data;

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

/**
 * @brief Write an fnode to disk.
 *
 * @param location on-disk location wher the fnode whould be written.
 * @param fnode the fnode to be written.
 */
int save_fnode(struct fnode_location_t *location, struct fnode *fnode) {
    uint8_t *sector_buffer;
    int error = 0;

    sector_buffer = object_alloc(SECTOR_SIZE);
    if (!sector_buffer)
       return -1;
    clear_buffer(sector_buffer, SECTOR_SIZE);

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

/**
 * @brief Extend a directory fnode's content (on-disk) by one dir_entry.
 *
 * @param dir_fnode_location
 * @param dir_fnode
 * @param new_entry
 */
int add_dir_entry(struct fnode_location_t *dir_fnode_location, struct fnode *dir_fnode, struct dir_entry *new_entry) {
    uint8_t *sector_buffer, *sector_buffer_backup, *dir_info_buffer_backup = NULL;
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
    sector_buffer_backup = object_alloc(2 * SECTOR_SIZE);
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
    memory_copy((char *) sector_buffer, (char *) sector_buffer_backup, SECTOR_SIZE);

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
    memory_copy((char *) sector_buffer, (char *) dir_info_buffer_backup, SECTOR_SIZE);

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

    goto exit_with_alloc;

undo_dir_info_sector_change:
    dir_fnode->size -= sizeof(struct dir_entry);
    write_to_storage_disk(dir_fnode->sector_indexes[0], SECTOR_SIZE, dir_info_buffer_backup);

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
    if (dir_info_buffer_backup)
        object_free(dir_info_buffer_backup);

exit:
    return error;
}

/**
 * @brief Remove a directory entry from a directory's content
 *
 * @param dir_fnode
 * @param name
 */
int remove_dir_entry(struct fnode *dir_fnode, char *name) {
    uint8_t *buffer = object_alloc(dir_fnode->size);
    struct fnode_location_t dir_fnode_location;
    int num_sectors_new, num_sectors_old, diff;
    const int name_len = strlen(name);
    struct dir_entry *dir_entry;
    struct dir_info *dir_info;
    int num_entries_to_shift;
    int error = 0, i;

    if (!buffer) {
        print_string("Error: object alloc failed in remove_dir_entry.\n");
        error = -1;
        goto return_error;
    }

    if (get_fnode_location(dir_fnode->id, &dir_fnode_location)) {
        print_string("Error removing dir_entry: get_fnode_location failed.\n");
        error = -1;
        goto free_buffer;
    }

    if (read_dir_content(dir_fnode, buffer) < 0) {
        print_string("Error removing dir_entry: failed read_dir_content.\n");
        error = -1;
        goto free_buffer;
    }

    dir_info = (struct dir_info*) buffer;
    dir_entry = (struct dir_entry*) (dir_info + 1);

    for (i = 0; i < dir_info->num_entries; i++, dir_entry++) {
        const int entry_name_len = strlen(dir_entry->name);

        if (entry_name_len == name_len &&
            strmatchn(name, dir_entry->name, name_len))
            break;
    }

    if (i == dir_info->num_entries) {
        print_string("Error removing entry: couldn't find name.\n");
        error = -1;
        goto free_buffer;
    }

    num_entries_to_shift = dir_info->num_entries - (i + 1);
    while (num_entries_to_shift--) {
        *dir_entry = *(dir_entry + 1);
        dir_entry++;
    }

    dir_info->num_entries--;

    const int new_size = (uint8_t *) dir_entry - (uint8_t *) dir_info;
    if (overwrite_dir_content(dir_fnode, buffer, new_size)) {
        print_string("Error adding dir_entry: overwrite_dir_content failed.\n");
        error = -1;
        goto free_buffer;
    }

    num_sectors_old = (dir_fnode->size / SECTOR_SIZE) +
                      ((dir_fnode->size % SECTOR_SIZE) ? 1 : 0);
    num_sectors_new = (new_size / SECTOR_SIZE) +
                      ((new_size % SECTOR_SIZE) ? 1 : 0);

    if ((diff = num_sectors_old - num_sectors_new)) {
        for (int i = 0; i < diff; i++)
            sector_bitmap_unset(dir_fnode->sector_indexes[num_sectors_new + i], 1);
    }

    dir_fnode->size = new_size;
    if (save_fnode(&dir_fnode_location, dir_fnode)) {
        print_string("Error: remove_dir_entry: save_fnode.\n");
        error = -1;
    }

free_buffer:
    object_free(buffer);

return_error:
    return error;
}

char *extract_filename_from_path(char *path) {
    int path_len = strlen(path);
    int i;

    if (path_len <= 0)
        return NULL;

    i = path_len - 1;

    if (path[i] == '/') {
        print_string("Error: filenames should not end with '/'.\n");
        return NULL;
    }

    // Skip past non '/'s.
    while(i && path[i] != '/')
        i--;

    if (path[i] == '/')
        i++;

    return &path[i];
}

/**
 * @brief Create a new file within the innermost directory of the directory chain
 * of the supplied filesystem context.
 *
 * @param ctx
 * @param file_info
 */
int create_file(struct fs_context *ctx, struct file_creation_info *file_info) {
    struct fnode_location_t parent_fnode_location, new_fnode_location;
    int sz = file_info->file_size, sz_sectors = 1;
    // TODO: We're allocating this on the stack because it'll probably be greater
    // than 2k which is that max dynamic object allocation allows, when we dynamic
    // allocation supports greater than 2k, we should use than instead of this
    // stack-allocation.
    int sector_indexes_buffer[MAX_FILE_CHUNKS];
    struct fnode parent_fnode, new_fnode;
    struct dir_entry new_dir_entry;
    struct directory_chain *chain;
    char *filename;
    int time, err;

    while ((sz_sectors << SECTOR_SIZE_SHIFT) < sz)
        sz_sectors++;

    if (sz_sectors > MAX_FILE_CHUNKS)
        return -1; // Unsupported file size.

    // Not really necesary, but just to avoid saving garbage from the stack.
    clear_buffer((uint8_t *) &new_fnode, sizeof(struct fnode));
    clear_buffer((uint8_t *) &new_dir_entry, sizeof(struct dir_entry));
    clear_buffer((uint8_t *) &new_fnode_location, sizeof(struct fnode_location_t));

    time_op(query_free_sectors(sz_sectors, sector_indexes_buffer), time, err);
    print_string("query_free_sectors took "); print_int32(time); print_string(" ticks.\n");
    if (err)
        return -1; // Not enough disk space.

    time_op(query_free_fnodes(1, &new_fnode_location), time, err);
    print_string("query_free_fnodes took "); print_int32(time); print_string(" ticks.\n");
    if (err)
        goto free_sectors; // Not enough fnodes.

    new_fnode.size = sz;
    new_fnode.type = FILE;
    new_fnode.id = NEXT_FNODE_ID++;
    for (int i = 0; i < sz_sectors; i++)
        new_fnode.sector_indexes[i] = sector_indexes_buffer[i];

    if (save_fnode(&new_fnode_location, &new_fnode))
        goto free_fnode;

    if (save_file(&new_fnode, file_info))
        goto unsave_new_fnode;

    filename = extract_filename_from_path(file_info->path);
    if (!filename) {
        print_string("Error create_file: extract_filename_from_path.\n");
        goto unsave_new_fnode;
    }

    memory_copy((char *)filename, new_dir_entry.name, strlen(filename));
    new_dir_entry.size = sz; // Unnecessary as size field is obsolete but leave for now.
    new_dir_entry.type = FILE;
    new_dir_entry.fnode_location = new_fnode_location;
    new_dir_entry.id = new_fnode.id;

    if (filename != file_info->path) {
        char c = *(filename -1);

        *(filename - 1) = '\0';
        chain = create_chain_from_path(ctx, file_info->path);
        *(filename - 1) = c;
    } else {
        chain = ctx->working_directory_chain;
    }

    if (!chain) {
        print_string("Error create_file: create_chain_from_path");
        goto unsave_new_fnode;
    }

    // We get the parent_fnode (and location) by this validation process.
    if (validate_directory_chain(chain, &parent_fnode, &parent_fnode_location)) {
        print_string("Error creating file: failed chain validation.\n");
        if (chain != ctx->working_directory_chain)
            destroy_directory_chain(chain);
        goto unsave_new_fnode;
    }

    if (add_dir_entry(&parent_fnode_location, &parent_fnode, &new_dir_entry))
        goto unsave_new_fnode;

    if (chain != ctx->working_directory_chain)
        destroy_directory_chain(chain);

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

int __delete_file(struct directory_chain *chain, char *deletion_target_name) {
    struct fnode_location_t enclosing_fnode_location, file_fnode_location;
    struct fnode enclosing_fnode, file_fnode;
    int to_delete, sector_idx;

    if (validate_directory_chain(chain,
                                 &enclosing_fnode,
                                 &enclosing_fnode_location)) {
        print_string("Error deleting file: couldn't validate chain.\n");
        return -1;
    }

    // Verify that target file is present in working directory.
    if (search_name_in_directory(deletion_target_name, &enclosing_fnode, &file_fnode)) {
        print_string("Error deleting file: couldn't find file in wd.\n");
        return -1;
    }

    // Free the sectors occupied by the file.
    to_delete = file_fnode.size;
    sector_idx = 0;
    while (to_delete) {
        int delete_size = to_delete > SECTOR_SIZE ? SECTOR_SIZE : to_delete;
        sector_bitmap_unset(file_fnode.sector_indexes[sector_idx++], 1ULL);
        to_delete -= delete_size;
    }

    // Free the fnode used by the file.
    if (get_fnode_location(file_fnode.id, &file_fnode_location)) {
        print_string("Error during delete: failed getting fnode location.\n");
        return -1;
    }

    fnode_bitmap_unset(file_fnode_location.fnode_table_index, 1);

    remove_dir_entry(&enclosing_fnode, deletion_target_name);

    return 0;
}

/**
 * @brief Delete a file from the current directory of an fs_context.
 *
 * @param ctx
 * @param folder_info
 */
int delete_file(struct fs_context *ctx, char *path) {
    char deletion_target_name[MAX_FILENAME_LENGTH];
    struct directory_chain *chain;
    int path_len = strlen(path);
    int i, j, error = 0;
    char c;

    if (path_len <= 0)
        return -1;

    i = path_len - 1;

    if (path[i] == '/') {
        print_string("Error: filenames should not end with (or have) '/'\n");
        return -1;
    }

    // Skip past non '/'s.
    j = i;
    while (i && path[i] != '/')
        i--;

    if (path[i] == '/')
        i++;

    clear_buffer((uint8_t *) deletion_target_name, MAX_FILENAME_LENGTH);

    memory_copy(&path[i], deletion_target_name, j - i + 1);

    // Mark end of enclosing path so it can be used to create a chain.
    c = path[i];
    path[i] = '\0';

    chain = create_chain_from_path(ctx, path);
    if (!chain) {
        print_string("Error: delete_file: create_chain_from_path.\n");
        error = -1;
    }
    path[i] = c;

    if (!error && __delete_file(chain, deletion_target_name)) {
        print_string("Error: delete_file: __delete_file.\n");
        error = -1;
    }

    if (chain)
        destroy_directory_chain(chain);

    return error;
}

static int free_dir_content_sectors(struct fnode *__fnode) {
    uint8_t *buffer = object_alloc(__fnode->size);
    struct dir_entry *dir_entry;
    struct dir_info *dir_info;

    if (!buffer) {
        print_string("Error free_dir_content: object_alloc.\n");
        return -1;
    }

    if (read_dir_content(__fnode, buffer) < 0) {
        print_string("Error free_dir_content_sectors: read_dir_content.\n");
        return -1;
    }

    dir_info = (struct dir_info *) buffer;
    dir_entry = (struct dir_entry *) (dir_info + 1);

    for (int i = 0; i < dir_info->num_entries; i++, dir_entry++) {
        int to_delete, sector_idx;
        struct fnode fnode;

        if (get_fnode_by_location(&dir_entry->fnode_location, &fnode)) {
            print_string("Error free_dir_content: get_fnode_by_location.\n");
            return -1;
        }

        if (dir_entry->type == FOLDER) {
            if (free_dir_content_sectors(&fnode)) {
                print_string("Error free_dir_content_sectors(fnode).\n");
                return -1;
            }
        }

        for (sector_idx = 0, to_delete = fnode.size; to_delete > 0; sector_idx++) {
            sector_bitmap_unset(fnode.sector_indexes[sector_idx], 1);
            to_delete -= to_delete > SECTOR_SIZE ? SECTOR_SIZE : to_delete;
        }

        fnode_bitmap_unset(dir_entry->fnode_location.fnode_table_index, 1);
    }

    return 0;
}

int validate_new_folder_name(char *foldername) {
    int name_len = strlen(foldername);
    int i = 0;

    // Verify that the new folder's name does not match '/[/]*'.
    while (i < name_len && foldername[i] == '/')
        i++;
    if (i == name_len)
        return -1;

    // TODO: Other folder name validation.

    return 0;
}

char *extract_foldername_from_path(char *path) {
    int path_len = strlen(path);
    int i;

    if (path_len <= 0)
        return NULL;

    i = path_len - 1;

    while (i && path[i] == '/')
        i--;

    // Skip past non '/'s.
    while(i && path[i] != '/')
        i--;

    return &path[i];
}

/**
 * @brief Create a new folder within the innermost directory of the supplied filesystem
 * context's chain.
 *
 * @param ctx
 * @param folder_info
 */
int create_folder(struct fs_context *ctx, struct folder_creation_info *folder_info) {
    struct fnode_location_t parent_fnode_location, new_fnode_location;
    int sz = sizeof(struct dir_info), sz_sectors = 1;
    // TODO: We're allocating this on the stack because it'll probably be greater
    // than 2k which is that max dynamic object allocation allows, when we dynamic
    // allocation supports greater than 2k, we should use than instead of this
    // stack-allocation.
    int sector_indexes_buffer[MAX_FILE_CHUNKS];
    struct fnode parent_fnode, new_fnode;
    struct dir_entry new_dir_entry;
    struct dir_info *new_dir_info;
    struct directory_chain *chain;
    int folderpath_len;
    char *foldername;
    int time, err;

    while ((sz_sectors << SECTOR_SIZE_SHIFT) < sz)
        sz_sectors++;

    if (sz_sectors > MAX_FILE_CHUNKS)
        return -1; // Unsupported file size.

    // Not really necesary, but just to avoid saving garbage from the stack.
    clear_buffer((uint8_t *) &new_fnode, sizeof(struct fnode));
    clear_buffer((uint8_t *) &new_dir_entry, sizeof(struct dir_entry));
    clear_buffer((uint8_t *) &new_fnode_location, sizeof(struct fnode_location_t));

    time_op(query_free_sectors(sz_sectors, sector_indexes_buffer), time, err);
    print_string("query_free_sectors took "); print_int32(time); print_string(" ticks.\n");
    if (err)
        return -1; // Not enough disk space.

    time_op(query_free_fnodes(1, &new_fnode_location), time, err);
    print_string("query_free_fnodes took "); print_int32(time); print_string(" ticks.\n");
    if (err)
        goto free_sectors; // Not enough fnodes.

    new_fnode.size = sizeof(struct dir_info);
    new_fnode.type = FOLDER;
    new_fnode.id = NEXT_FNODE_ID++;
    for (int i = 0; i < sz_sectors; i++)
        new_fnode.sector_indexes[i] = sector_indexes_buffer[i];

    if (save_fnode(&new_fnode_location, &new_fnode))
        goto free_fnode;

    foldername = extract_foldername_from_path(folder_info->path);
    if (!foldername) {
        print_string("Error create_folder: bad path?\n");
        goto free_fnode;
    }

    if (validate_new_folder_name(foldername)) {
        print_string("Error create_folder: invalid new folder name.\n");
        goto free_fnode;
    }

    // The 2 if/else blocks that follow are a little esoteric, so here's an
    // explanation for posterity:
    //
    // folder_info->path is a stack allocated array of chars.
    // foldername points to the byte/char in folder_info->path which starts
    // the name of the folder being added.
    //
    // The foldername could potentially include trailing '/'s. So, the first
    // if block tries to set mark the end of the string at the first of the
    // (possibly) trailing '/'s.
    //
    // To create the chain, we need to mark the end of the path correctly.
    // If the folder's name (foldername) is the same as the beginning of the
    // path, then the path contains just the folder name (no '/'s) and the
    // folder is to be created in the working directory of the given context.
    // Otherwise, '/'s are included in the path - in this case, we need to be
    // mindful of paths that begin with '/'s - the inner if in the second
    // if/else block handles this by terminating the path at the beginning of
    // the foldername (this allows a path like "/home" to temporarily become
    // "/\0ome" so we can correctly create a chain of just "/". This also
    // works for something like "////home" (=> "////\0ome") as the
    // create_chain_from_path knows how to deal with the extra '/'s.
    // The inner else handles the case where the '/' that precede filename
    // do not start the path, e.g "home////docs".
    folderpath_len = strlen(folder_info->path);
    if (folder_info->path[folderpath_len - 1] == '/') {
        int i = folderpath_len - 1;

        while (folder_info->path[i] == '/') {
            i--;
        }

        folder_info->path[i + 1] = '\0';
    }

    if (foldername != folder_info->path) {
        char *cp = foldername - 1;
        char c;

        while (cp > folder_info->path && *cp == '/')
            cp--;

        if (cp == folder_info->path) {
            c = *foldername;
            *foldername = '\0';
            chain = create_chain_from_path(ctx, folder_info->path);
            *foldername = c;
        } else {
            c = *(foldername - 1);
            *(foldername - 1) = '\0';
            chain = create_chain_from_path(ctx, folder_info->path);
            *(foldername - 1) = c;
        }
    } else {
        chain = ctx->working_directory_chain;
    }

    if (!chain) {
       print_string("Error: create_folder: NULL chain?\n");
       goto free_fnode;
    }

    // Prepare the folder info so we can save it. This writes the dir_info data
    // which begins every folder's content.
    folder_info->data = object_alloc(sizeof(struct dir_info));
    clear_buffer((uint8_t *) folder_info->data, sizeof(struct dir_info));
    folder_info->size = sizeof(struct dir_info);

    new_dir_info = (struct dir_info*) folder_info->data;
    new_dir_info->num_entries = 0;
    memory_copy((char *)foldername, (char *) &new_dir_info->name, strlen(foldername));

    // Save the folder (currently containing only a dir_info).
    if (save_folder(&new_fnode, folder_info)) {
        object_free(folder_info->data);
        goto unsave_new_fnode;
    }
    object_free(folder_info->data);

    // We get the parent_fnode (and location) by this validation process.
    if (validate_directory_chain(chain, &parent_fnode, &parent_fnode_location)) {
        print_string("Error creating folder: failed chain validation.\n");
        goto unsave_new_fnode;
    }

    // Prepare the dir_entry which will be added to the folder's parent folder.
    memory_copy((char *)foldername, new_dir_entry.name, strlen(foldername));
    new_dir_entry.size = sizeof(struct dir_info); // Unnecessary as size field is obsolete but leave for now.
    new_dir_entry.type = FOLDER;
    new_dir_entry.fnode_location = new_fnode_location;
    new_dir_entry.id = new_fnode.id;
    if (add_dir_entry(&parent_fnode_location, &parent_fnode, &new_dir_entry))
        goto unsave_new_fnode;

    if (chain != ctx->working_directory_chain)
        destroy_directory_chain(chain);
    return 0;

    // It's not really necessary to unsave a new fnode. We only need to mark the
    // bit free in the fnode bitmap.
unsave_new_fnode:
    unsave_fnode(new_fnode_location);

if (chain != ctx->working_directory_chain)
        destroy_directory_chain(chain);

free_fnode:
    fnode_bitmap_unset(new_fnode_location.fnode_table_index, 1);

free_sectors:
    for (int i = 0; i < sz_sectors; i++)
        sector_bitmap_unset(sector_indexes_buffer[i], 1);

    return -1;
}

static int __delete_folder(struct directory_chain *chain, char *deletion_target_name) {
    struct fnode_location_t enclosing_fnode_location, folder_fnode_location;
    struct fnode enclosing_fnode, folder_fnode;
    int to_delete, sector_idx;

    if (validate_directory_chain(chain,
                                 &enclosing_fnode,
                                 &enclosing_fnode_location)) {
        print_string("Error deleting folder: couldn't validate chain.\n");
        return -1;
    }

    if (search_name_in_directory(deletion_target_name, &enclosing_fnode, &folder_fnode)) {
        print_string("Error deleting folder: couldn't find folder in enclosing directory.\n");
        return -1;
    }

    if (free_dir_content_sectors(&folder_fnode)) {
        print_string("Error deleting folder: content sectors free failed.\n");
        return -1;
    }

    // Free the sectors occupied by the folder.
    to_delete = folder_fnode.size;
    sector_idx = 0;
    while (to_delete) {
        int delete_size = to_delete > SECTOR_SIZE ? SECTOR_SIZE : to_delete;
        sector_bitmap_unset(folder_fnode.sector_indexes[sector_idx++], 1ULL);
        to_delete -= delete_size;
    }

    // Free the fnode used by the folder.
    if (get_fnode_location(folder_fnode.id, &folder_fnode_location)) {
        print_string("Error during delete: failed getting fnode location.\n");
        return -1;
    }

    fnode_bitmap_unset(folder_fnode_location.fnode_table_index, 1);

    remove_dir_entry(&enclosing_fnode, deletion_target_name);

    return 0;
}

int delete_folder(struct fs_context *ctx, char *path) {
    char deletion_target_name[MAX_FILENAME_LENGTH];
    struct directory_chain *chain;
    int path_len = strlen(path);
    int i, j, error = 0;
    char c;

    if (path_len <= 0)
        return -1;

    i = path_len - 1;

    while (i >= 0 && path[i] == '/')
        i--;

    if (i < 0) {
        print_string("Deleting '/' (the root directory) is not allowed.\n");
        return -1;
    }

    j = i;
    while (i && path[i] != '/')
        i--;

    if (path[i] == '/')
        i++;

    clear_buffer((uint8_t *) deletion_target_name, MAX_FILENAME_LENGTH);
    memory_copy((char *)&path[i], deletion_target_name, j - i + 1);

    c = path[i];
    path[i] = '\0';

    chain = create_chain_from_path(ctx, path);
    if (!chain) {
        print_string("Error: delete_file: create_chain_from_path.\n");
        error = -1;
    }
    path[i] = c;

    if (!error && __delete_folder(chain, deletion_target_name)) {
        print_string("Error: delete_file: __delete_file.\n");
        error = -1;
    }

    if (chain)
        destroy_directory_chain(chain);

    return error;
}

/**
 * @brief Read the content of directory associated with the provided fnode into the provided
 * buffer.
 *
 * @param dir_fnode
 * @param buffer
 */
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

/**
 * @brief Overwrite the content of a directory fnode.
 *
 * @param fnode
 * @param buffer
 * @param bytes
 */
int overwrite_dir_content(struct fnode *fnode, uint8_t *buffer, int bytes) {
    int error = 0, written = 0, to_write, sectors_written = 0;
    uint8_t *data = buffer;

    // Write to disk one sector at a time because fnode->sector_indexes
    // might not be contiguous.
    while (written < bytes) {
        int idx = fnode->sector_indexes[sectors_written];

        to_write = (bytes - written) < SECTOR_SIZE ? (bytes - written) : SECTOR_SIZE;
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

/**
 * @brief Pretty-print an fnode_location struct.
 *
 * @param location
 */
void print_fnode_location(const struct fnode_location_t *location) {
    print_string("[ ");
    print_int32(location->fnode_table_index);
    print_string(", ");
    print_int32(location->fnode_sector_index);
    print_string(", ");
    print_int32(location->offset_within_sector);
    print_string(" ]");
}

/**
 * @brief Pretty-print a dir_entry struct.
 *
 * @param dir_entry
 */
void print_dir_entry(const struct dir_entry *dir_entry) {
    print_string("dir-entry: ");
    print_string(dir_entry->name); print_string(" ");
    print_fnode_location(&dir_entry->fnode_location); print_string("\n");
}

/**
 * @brief List the contents of the directory pointed to by the given fnode.
 *
 * @param dir_fnode
 */
void show_dir_content(const struct fnode *dir_fnode) {
    struct dir_entry *dir_entry;
    struct dir_info *dir_info;
    uint8_t *buffer;

    buffer = object_alloc(dir_fnode->size);
    if (read_dir_content(dir_fnode, buffer) < 0) {
        print_string("Unable to read dir content.\n");
        return;
    }

    dir_info = (struct dir_info*) buffer;
    dir_entry = (struct dir_entry *) (buffer + sizeof(struct dir_info));
    for (int i = 0; i < dir_info->num_entries; i++, dir_entry++) {
        switch (dir_entry->type)
        {
        case FILE:
            print_string("fi ");
            break;
        case FOLDER:
            print_string("fo ");
            break;
        default:
            print_string("unknown-type ");
            break;
        }
        print_string("("); print_int32(i); print_string(") "); print_dir_entry(dir_entry);
    }

    object_free(buffer);
}

/**
 * @brief Read in the fnode of the provided directory entry.
 *
 * @param fnode_ptr
 */
int get_fnode_by_location(struct fnode_location_t *location, struct fnode* fnodep) {
    uint8_t *buffer = object_alloc(SECTOR_SIZE);

    // Read fnode in from disk.
    if (read_from_storage_disk(location->fnode_sector_index, SECTOR_SIZE, buffer))
        return -1;

    *fnodep = *((struct fnode*)(buffer + location->offset_within_sector));

    object_free(buffer);

    return 0;
}

/**
 * @brief Read in the fnode of the provided directory entry.
 *
 * @param fnode_ptr
 */
int get_fnode(struct dir_entry *entry, struct fnode* fnode_ptr) {
    uint8_t *buffer = object_alloc(SECTOR_SIZE);

    // Read fnode in from disk.
    if (read_from_storage_disk(entry->fnode_location.fnode_sector_index, SECTOR_SIZE, buffer))
        return -1;

    *fnode_ptr = (struct fnode)(*(struct fnode*)(buffer + entry->fnode_location.offset_within_sector));

    object_free(buffer);

    return 0;
}

/**
 * @brief Get the location of fnode with id id within the folder represented by fnode
 *
 * CAREFUL! The return value semantics are:
 *
 *  < 0 => an error occurred
 * == 0 => no error occurred but the fnode was not found
 *  > 0 => fnode with the required id was found
 *
 * @param fnode (MUST HAVE type=FOLDER)
 * @param id
 * @param result_fnode_location
 */
int __get_fnode_location(struct fnode *fnode, fnode_id_t id, struct fnode_location_t *result_fnode_location) {
    uint8_t *buffer = object_alloc(fnode->size);
    struct dir_entry *dir_entry;
    struct dir_info *dir_info;
    int error = 0, i;

    if (read_dir_content(fnode, buffer) < 0) {
        print_string("Error: __get_fnode_location: read_dir_content.\n");
        error = -1;
        goto free_buffer;
    }

    dir_info = (struct dir_info *) buffer;
    dir_entry = (struct dir_entry *) (dir_info + 1);

    for (i = 0; i < dir_info->num_entries; i++, dir_entry++) {
        if (dir_entry->id == id)
            break;
    }

    if (i < dir_info->num_entries) {
        *result_fnode_location = dir_entry->fnode_location;
        error = 1;
        goto free_buffer;
    }

    // Since we didn't find the id in the current directory, we must recursively search its subdirectories.
    dir_entry = (struct dir_entry *) (dir_info + 1);

    for (i = 0; i < dir_info->num_entries; i++, dir_entry++) {
        struct fnode subdir_fnode;

        if (dir_entry->type != FOLDER)
            continue;

        if (get_fnode(dir_entry, &subdir_fnode)) {
            print_string("Error __get_fnode_location: get_fnode.\n");
            error = -1;
            goto free_buffer;
        }

        // TODO: Do we really care if a subdirectory search encounters an error?
        // Such an error is not likely to have downstream effects so at least
        // for now we can just ignore it.
        if (__get_fnode_location(&subdir_fnode, id, result_fnode_location) > 0) {
            error = 1;
            break;
        }
    }

free_buffer:
    object_free(buffer);

    return error;
}

int get_fnode_location(fnode_id_t id, struct fnode_location_t *result_fnode_location) {
    struct fnode root_fnode;

    if (id == root_dir_entry.id) {
        *result_fnode_location = root_dir_entry.fnode_location;
        return 0;
    }

    if (get_fnode(&root_dir_entry, &root_fnode)) {
        print_string("Error get_fnode_location: get_fnode.\n");
        return -1;
    }

    if (__get_fnode_location(&root_fnode, id, result_fnode_location) <= 0) {
        print_string("Error get_fnode_location: __get_fnode_location.\n");
        return -1;
    }

    return 0;
}

int get_fnode_by_id(fnode_id_t id, struct fnode *fnode) {
    struct fnode_location_t location;

    if (get_fnode_location(id, &location)) {
        print_string("Error: get_fnode: get_fnode_location.\n");
        return -1;
    }

    if (get_fnode_by_location(&location, fnode)) {
        print_string("Error: get_fnode: get_fnode.\n");
        return -1;
    }

    return 0;
}

int get_dir_info_from_chain(struct directory_chain *chain, struct dir_info *dir_info) {
    struct fnode_location_t fnode_location;
    struct fnode fnode;

    if (validate_directory_chain(chain, &fnode, &fnode_location)) {
        print_string("Error getting dir_info: chain validation failed.\n");
        return -1;
    }

    return get_dir_info(&fnode, dir_info);
}

/**
 * @brief Read in the info of the directory pointed to by the given fnode.
 *
 * @param fnode
 * @param output buffer where the struct dir_info is written.
 */
int get_dir_info(struct fnode *fnode, struct dir_info *dir_info) {
    return read_from_storage_disk(fnode->sector_indexes[0], sizeof(struct dir_info), (uint8_t *) dir_info);
}

/**
 * @brief Set on the sector_bitmap the corresponding bits of the sectors occupied
 * by this fnode's content.
 *
 * @param _fnode
 */
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
    if (read_dir_content(_fnode, buffer) < 0) {
        print_string("Error initializing usage bits for fnode at "); print_ptr(_fnode);
        print_string(".\n");
        return;
    }

    if (_fnode->id >= NEXT_FNODE_ID)
        NEXT_FNODE_ID = _fnode->id + 1;

    dir_info = (struct dir_info*) buffer;
    dir_entry = (struct dir_entry *) (buffer + sizeof(struct dir_info));
    for (int i = 0; i < dir_info->num_entries; i++, dir_entry++) {
        char sector_buffer[SECTOR_SIZE];
        struct fnode *__fnode;

        // Mark the fnode used by this entry in the fnode bitmap.
        fnode_bitmap_set(dir_entry->fnode_location.fnode_table_index, 1);

        // Read in the sector containing the fnode for this dir_entry.
        read_from_storage_disk(dir_entry->fnode_location.fnode_sector_index, SECTOR_SIZE, &sector_buffer);
        __fnode = (struct fnode *) &sector_buffer[dir_entry->fnode_location.offset_within_sector];

        // Mark the sectors occupied by this dir_entry's content.
        record_fnode_sector_bits(__fnode);

        if (__fnode->id >= NEXT_FNODE_ID)
            NEXT_FNODE_ID = __fnode->id + 1;

        // If it's a directory, recurse, so we can account for its children.
        if (dir_entry->type == FOLDER)
            __init_usage_bits(__fnode);
    }

    object_free(buffer);
}

/**
 * @brief Initialize the sector_bitmap and the fnode_bitmap by setting the bits
 * for the files that any files that are already on the system.
 *
 */
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

/**
 * @brief Read in the root fnode struct.
 *
 */
void init_root_fnode(void) {
    root_dir_entry.fnode_location = master_record.root_dir_fnode_location;
    if (get_fnode(&root_dir_entry, &root_fnode)) {
        print_string("Error getting root_fnode.");
        return;
    }
    root_dir_entry.size = root_fnode.size;
}

/**
 * @brief Initialize the master_record.
 *
 */
void init_master_record(void) {
    read_from_storage_disk(0, sizeof(struct fs_master_record), &master_record);
}

/**
 * @brief Initialize the filesystem.
 *
 */
void init_fs(void) {
    init_master_record();

    init_root_fnode();

    init_usage_bits();
}
