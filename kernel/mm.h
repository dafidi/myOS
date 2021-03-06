#ifndef __MM_H__
#define __MM_H__

#include "system.h"

#define KERNEL_CODE_SEGMENT_IDX 	SYSTEM_GDT_KERNEL_CODE_IDX
#define KERNEL_DATA_SEGMENT_IDX 	SYSTEM_GDT_KERNEL_DATA_IDX
#define USER_CODE_SEGMENT_IDX 		SYSTEM_GDT_USER_CODE_IDX
#define USER_DATA_SEGMENT_IDX		SYSTEM_GDT_USER_DATA_IDX
#define KERNEL_TSS_DESCRIPTOR_IDX	SYSTEM_KERNEL_TSS_DESCRIPTOR_IDX
#define USER_TSS_DESCRIPTOR_IDX		SYSTEM_USER_TSS_DESCRIPTOR_IDX

#define PAGE_SIZE 0x1000
#define PAGE_ADDR_MASK (~(PAGE_SIZE - 1))
#define PAGE_ALIGN(x) ((x) & PAGE_ADDR_MASK)
#define PAGE_ALIGN_UP(x) (PAGE_ALIGN(x) == (x) ? (x) : PAGE_ALIGN(x) + PAGE_SIZE)

#define USER_PAGE_TABLE_SIZE 1024
#define USER_PAGE_DIR_SIZE 1024

struct bios_mem_map {
	unsigned long long base;
	unsigned long long length;
	unsigned int type;
	unsigned int acpi_null;
}__attribute__((packed));

struct gdt_entry {
	unsigned short limit0_15;
	unsigned short base0_15;
	unsigned char base16_23;
	unsigned char type_s_dpl_p;
	unsigned char limit16_19_avl_l_db_g;
	unsigned char base24_31;
}__attribute__((packed));

struct gdt_info {
	unsigned short len;
	unsigned long addr;
}__attribute__((packed));

typedef unsigned int pte_t;
typedef unsigned long long va_t;
typedef unsigned long long va_range_sz_t;
typedef unsigned long pa_t;
typedef unsigned long pa_range_sz_t;

void init_mm(void);

unsigned int get_available_memory(void);
int reserve_and_map_user_memory(va_t va, unsigned int pa, unsigned int amount);
int unreserve_and_unmap_user_memory(va_t va, pa_t pa, unsigned int amount);
void make_gdt_entry(struct gdt_entry* entry,
					unsigned int limit,
					unsigned int base,
					char type,
					/*flags format: S_DPL_P_AVL_L_DB_G*/
					/*bits:         1_2___1_1___1_1__1*/
					char flags);
#endif // __MM_H__
