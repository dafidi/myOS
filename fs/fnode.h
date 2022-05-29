
#ifndef __FNODE_H__
#define __FNODE_H__

#define KiB (1024)
#define MiB (1024 * KiB)
#define GiB (1024 * MiB)

#define INT_SIZE_SHIFT 2

#define MAX_FILE_SIZE 1 << 20
#define MAX_FILE_CHUNKS ((MAX_FILE_SIZE) >> SECTOR_SIZE_SHIFT)

enum fnode_type {
    FILE,
    FOLDER
};

typedef uint32_t fblock_index_t;

// This is the first piece of data in a directory fnode's content.
// So every directory fnode has a size of at least sizeof(struct dir_info).
struct dir_info {
    uint32_t num_entries;
};

struct fnode {
    uint32_t size;                  // The size of the file or folder.
    enum fnode_type type;           // FILE or FOLDER.
    uint8_t reserved[60];
    fblock_index_t sector_indexes[15];     // TODO: Treat 13 and 14 as singly and doubly indirect respectively.
                                           // For now, singly indirect only -> MAX_FILE_SIZE=7689. (not too bad.)
}__attribute__((packed));

struct fnode_location_t {
    uint32_t fnode_table_index;     // Index into the global array of fnodes.
    uint32_t fnode_sector_index;    // fnode index of file or folder.
    uint16_t fnode_sector_offset;   // Byte offset within the containing sector where the fnode for this file exists.
}__attribute__((packed)); // sizeof: 4 + 4 + 2 = 10

struct dir_entry {
    char name[128];                     // Name of file or folder.
    uint32_t size;                      // Size of file or folder.
    enum fnode_type type;               // FILE or FOLDER.
    struct fnode_location_t fnode_location;   // fnode fnode_location descriptor.
 }__attribute__((packed)); // sizeof = 128 + 4 + 4 + 10 = 146

struct fs_master_record {
    struct fnode_location_t root_dir_fnode_location;  // Index or block containing fnode of root folder.
    uint32_t fnode_bitmap_start_sector;
    uint32_t fnode_bitmap_size;
    uint32_t fnode_table_start_sector;
    uint32_t sector_bitmap_start_sector;
    uint32_t sector_bitmap_size;
    uint32_t data_blocks_start_sector;
}__attribute__((packed));

void init_fs(void);

#endif
