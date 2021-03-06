/**
 * In this filesystem design, disk sectors are "atomic", i.e.
 * used to store only one thing whether it's file content or
 * file/folder descriptor.
 */

#ifndef __FNODE_H__
#define __FNODE_H__

#define KiB (1024)
#define MiB (1024 * KiB)
#define GiB (1024 * MiB)

#define SECTOR_SIZE_SHIFT 9
#define INT_SIZE_SHIFT 2

#define MAX_FILE_SIZE 1 << 20
#define MAX_FILE_CHUNKS (MAX_FILE_SIZE) >> SECTOR_SIZE_SHIFT

enum folder_content_type {
    FILE,
    FOLDER
};

struct file_chunk {
    int block;
    int offset;
    int len;
};

struct file {
    char name[128];
    int num_chunks;
    struct file_chunk chunks[MAX_FILE_CHUNKS];
};

struct folder_content_desc {
    int block;
    int offset;
    int len;
    enum folder_content_type type;
};

struct master_record {
    char name[128];
    struct folder_content_desc root_folder_desc;
};

struct folder {
    char name[128];
    int num_contents;
    struct folder_content_desc content[];
};

struct disk_block_desc {
    unsigned short size;
    unsigned short reserved;
};

struct folder_content_desc root_folder_desc;
struct master_record __master_record;
struct folder root_folder;

enum sys_error init_fs(void);

int find_file(struct folder *folder, char *file_name, struct file *file);
/**
 * find_file - XXX.
 */
int find_folder(struct folder *folder, char *folder_name, struct folder *target_folder);

void execute_binary_file(struct file *file);

#endif
