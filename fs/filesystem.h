#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include <kernel/system.h>

#define BITS_PER_BYTE 8

#define INT_SIZE_SHIFT 2

#define MAX_FILENAME_LENGTH 127
#define MAX_FILE_SIZE 1 << 20
#define MAX_FILE_CHUNKS ((MAX_FILE_SIZE) >> SECTOR_SIZE_SHIFT)

typedef uint32_t fblock_index_t;
typedef uint32_t fnode_id_t;

enum fnode_type {
    FILE,
    FOLDER
};

// This is the first piece of data in a directory fnode's content.
// So every directory fnode has a size of at least sizeof(struct dir_info).
struct dir_info {
    char name[MAX_FILENAME_LENGTH + 1];
    uint32_t num_entries;
};

struct fnode {
    fnode_id_t id;                  // Filesystem-wide id number for this file/folder.
    uint32_t size;                  // The size of the file or folder.
    enum fnode_type type;           // FILE or FOLDER.
    uint8_t reserved[56];
    fblock_index_t sector_indexes[15];     // TODO: Treat 13 and 14 as singly and doubly indirect respectively.
                                           // For now, singly indirect only -> MAX_FILE_SIZE=7689. (not too bad.)
}__attribute__((packed));

struct fnode_location_t {
    uint32_t fnode_table_index;     // Index into the global array of fnodes.
    uint32_t fnode_sector_index;    // fnode index of file or folder.
    uint16_t offset_within_sector;  // Byte offset within the containing sector where the fnode for this file exists
                                    // (There maybe more than 1 (currently 4) fnodes strored within a sector).
}__attribute__((packed)); // sizeof: 4 + 4 + 2 = 10

struct dir_entry {
    char name[MAX_FILENAME_LENGTH + 1]; // Name of file or folder.
    uint32_t size;                      // Size of file or folder - Obsolete, do not use.
    enum fnode_type type;               // FILE or FOLDER.
    struct fnode_location_t fnode_location;   // fnode fnode_location descriptor.
    fnode_id_t id;
 }__attribute__((packed)); // sizeof = 128 + 4 + 4 + 10 + 4 = 150

struct fs_master_record {
    struct fnode_location_t root_dir_fnode_location;  // Index or block containing fnode of root folder.
    uint32_t fnode_bitmap_start_sector;
    uint32_t fnode_bitmap_size;
    uint32_t fnode_table_start_sector;
    uint32_t sector_bitmap_start_sector;
    uint32_t sector_bitmap_size;
    uint32_t data_blocks_start_sector;
}__attribute__((packed));

struct file_creation_info {
    char path[MAX_FILENAME_LENGTH];
    uint8_t *file_content;
    int file_size;
};

struct file_deletion_info {
    char path[MAX_FILENAME_LENGTH];
};

struct folder_creation_info {
    char path[MAX_FILENAME_LENGTH];
    uint8_t *data;
    int size;
};

struct folder_deletion_info {
    char name[MAX_FILENAME_LENGTH];
};

struct directory_chain_link {
    struct directory_chain_link *next;
    struct directory_chain_link *prev;
    fnode_id_t id;
    char *name;
};

struct directory_chain {
    struct directory_chain_link *head; // This should always be root.
    struct directory_chain_link *tail; // This should always point to
                                                     // the current working directory.
    int size;                          // Shortcut way to get the number of links.
};

struct fs_context {
    struct fnode *curr_dir_fnode;
    struct fnode_location_t curr_dir_fnode_location;
    struct directory_chain *working_directory_chain;
};


struct directory_chain *create_chain_from_path(struct fs_context *, char*);
int push_directory_chain_link(struct directory_chain *, struct fnode *);
int pop_directory_chain_link(struct directory_chain *);
struct directory_chain *init_directory_chain(void);
void destroy_directory_chain(struct directory_chain *);
int create_file(struct fs_context *, struct file_creation_info *);
int delete_file(struct fs_context *, char *);
int create_folder(struct fs_context *, struct folder_creation_info *);
int delete_folder(struct fs_context *, char *);
int fs_search(struct directory_chain *, char*, struct fnode *);
int list_dir_content(struct fs_context *, char *);
int get_dir_info_from_chain(struct directory_chain *, struct dir_info *);
int get_dir_info(struct fnode *, struct dir_info *);
int get_fnode_by_id(fnode_id_t, struct fnode*);
int get_fnode(struct dir_entry *, struct fnode *);
int get_fnode_by_location(struct fnode_location_t *, struct fnode *);
int get_fnode_location(fnode_id_t, struct fnode_location_t *);
int read_dir_content(const struct fnode *, uint8_t *);
int overwrite_dir_content(struct fnode *, uint8_t *, int);
void show_dir_content(const struct fnode *);
void init_fs(void);

#endif
