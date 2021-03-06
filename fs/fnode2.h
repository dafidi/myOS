/**
 * In this filesystem design, disk sectors are "atomic", i.e.
 * used to store only one thing whether it's file content or
 * file/folder descriptor.
 */

#ifndef __FNODE2_H__
#define __FNODE2_H__

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
    struct file_chunk chunks[];
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

#define GiB (1024 * MiB)
#define MiB (1024 * KiB)
#define KiB (1024)

#define SECTOR_SIZE_SHIFT 9
#define INT_SIZE_SHIFT 2

struct disk_block_desc {
    unsigned short size;
    unsigned short reserved;
};

struct folder_content_desc root_folder_desc;
struct master_record __master_record;
struct folder root_folder;

enum sys_error init_fs(void);

// These are used where we need to dynamically provide file/folder.
// Ideally, we'd want dynamic memory allocation instead.
struct file buffer_file;
struct folder buffer_folder;

struct file *find_file(struct folder *folder, char *file_name);
/**
 * find_file - XXX.
 */
struct folder *find_folder(struct folder *folder, char *folder_name);

void execute_binary_file(struct file *file);

#endif
