#ifndef __MM_H__

#include "system.h"

#define PAGE_SIZE 0x1000
#define PAGE_ADDR_MASK (~(PAGE_SIZE - 1))

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

void init_mm(void);
void setup_pm_gdt(void);
void show_gdt(struct gdt_entry* gdt, int num_entries);
void make_gdt_entry(struct gdt_entry* entry, unsigned int limit, unsigned int base, char type, char flags);
void load_pm_gdt(void);

typedef unsigned int pte_t;
typedef long long unsigned int va_t;
typedef long long unsigned int va_range_sz_t;

void setup_page_directory_and_page_tables(void);
#endif // __MM_H__
