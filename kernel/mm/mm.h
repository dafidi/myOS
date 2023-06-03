#ifndef __MM_H__
#define __MM_H__

#include <kernel/system.h>

#include "zone.h"

#define KERNEL_CODE_SEGMENT_IDX 	SYSTEM_GDT_KERNEL_CODE_IDX
#define KERNEL_DATA_SEGMENT_IDX 	SYSTEM_GDT_KERNEL_DATA_IDX
#define USER_CODE_SEGMENT_IDX 		SYSTEM_GDT_USER_CODE_IDX
#define USER_DATA_SEGMENT_IDX		SYSTEM_GDT_USER_DATA_IDX
#define KERNEL_TSS_DESCRIPTOR_IDX	SYSTEM_KERNEL_TSS_DESCRIPTOR_IDX
#define USER_TSS_DESCRIPTOR_IDX		SYSTEM_USER_TSS_DESCRIPTOR_IDX

#define USER_PAGE_TABLE_SIZE 1024
#define USER_PAGE_DIR_SIZE 1024

// Memory object stuff
#define MIN_MEMORY_OBJECT_ORDER 5
#define MAX_MEMORY_OBJECT_ORDER 11
#define MEMORY_OBJECT_ORDER_RANGE MAX_MEMORY_OBJECT_ORDER - MIN_MEMORY_OBJECT_ORDER

struct gdt_entry {
	unsigned short limit0_15;
	unsigned short base0_15;
	unsigned char base16_23;
	unsigned char type_s_dpl_p;
	unsigned char limit16_19_avl_l_db_g;
	unsigned char base24_31;
}__attribute__((packed));

struct gdt64_tss_entry {
	unsigned short limit0_15;
	unsigned short base0_15;
	unsigned char base16_23;
	unsigned char type_s_dpl_p;
	unsigned char limit16_19_avl_l_db_g;
	unsigned char base24_31;
	unsigned int base32_63;
}__attribute__((packed));

struct gdt_info {
	unsigned short len;
	unsigned long addr;
}__attribute__((packed));
struct memory_object_header {
	struct memory_object *next;
	int order;
	int size;
}__attribute__((packed));

struct memory_object {
	struct memory_object_header header;
};

struct memory_object_cache {
	struct mem_block *object_block_ptr;
	struct memory_object *free_objects;
	struct memory_object *used_objects;
	uint16_t object_size;
	int order;
	int free;
	int used;
};

void make_gdt_entry(struct gdt_entry* entry,
					unsigned int limit,
					unsigned int base,
					char type,
					/*flags format: S_DPL_P_AVL_L_DB_G*/
					/*bits:         1_2___1_1___1_1__1*/
					char flags);
void make_gdt64_tss_entry(struct gdt64_tss_entry* entry,
                    unsigned int limit,
                    uint64_t base,
                    char type,
                    /*flags format: S_DPL_P_AVL_L_DB_G*/
                    /*bits:         1_2___1_1___1_1__1*/
                    char flags);
unsigned int get_available_memory(void);
int reserve_and_map_user_memory(va_t va, pa_t pa, unsigned int amount);
int unreserve_and_unmap_user_memory(va_t va, pa_t pa, unsigned int amount);

struct mem_block *zone_alloc(const int amt);
void zone_free(struct mem_block *block);

uint8_t* object_alloc(int amt);
void object_free(uint8_t *va);

void init_mm(void);

#endif // __MM_H__
