#ifndef __FNODE_H__
#define __FNODE_H__
/**
 * WIP!
 */
#include <kernel/system.h>
#include <drivers/disk/disk.h>

struct file_node {
	lba_t first_block;
	lba_t last_block;
	char name[256];
	uint32_t id;
}__attribute__((packed));

struct folder_node {
	lba_t first_block;
	lba_t last_block;
	uint32_t num_contents;
	char name[256];
	uint32_t id;
	uint32_t* folder_id_list;
}__attribute__((packed));

#endif /* __FNODE_H__ */
