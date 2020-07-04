#include "mm.h"

#include "drivers/screen/screen.h"
#include "string.h"

// System memory map as told by BIOS.
extern unsigned int mem_map_buf_addr;
extern unsigned int mem_map_buf_entry_count;

// Kernel uninitialized data start and end.
extern int _bss_start;
extern int _bss_end;

// Memory management structures for the kernel's use.
static uint32_t kernel_mem_bitmap;
static struct bios_mem_map* bmm;

// Kernel structures for segmentation.
struct gdt_entry pm_gdt[5];
struct gdt_info pm_gdt_info = {
		.len = sizeof(pm_gdt),
		.addr = pm_gdt,
};

// Kernel structures for paging.
// We have 1024 page directory entries -> 1024 page tables and each page table has 1024 ptes.
unsigned int kernel_page_directory[1024]__attribute__((aligned(0x1000)));
unsigned int kernel_page_tables[1024][1024];
extern void setup_and_enable_paging(void);

void init_mm(void) {
  	print("mem_map_buf_addr="); print_int32(mem_map_buf_addr);
	print("\n");
	print("mem_map_buf_entry_count="); print_int32(mem_map_buf_entry_count);
	print("\n");

	bmm = (struct bios_mem_map*) mem_map_buf_addr;

	for (int i = 0; i < mem_map_buf_entry_count; i++) {
		print("entry "); print_int32(i); print(" has base "); print_int32( bmm[i].base);
    	print(" and length "); print_int32( bmm[i].length); print(".\n");
	}

	kernel_mem_bitmap = ~((unsigned int) 0);
	print("kernel_mem_bitmap="); print_int32(kernel_mem_bitmap); print("\n");

	print("_bss_start="); print_int32((unsigned int) &_bss_start); print("\n");
	print("_bss_end="); print_int32((unsigned int) &_bss_end); print("\n");
	
	setup_pm_gdt();
	setup_page_directory_and_page_tables();
	setup_and_enable_paging();
}

void setup_page_directory_and_page_tables(void) {
	unsigned int addr = (unsigned int) &kernel_page_tables[0];
	unsigned int i, j, page_frame = 0;

	for (i = 0; i < 1024; i++) {
		// Set all PDEs.
		kernel_page_directory[i] = addr & 0xfffff000;
		kernel_page_directory[i] |= 0x3;
		addr += 0x1000;
	}

	for (i = 0; i < 1024; i++) {
		for (j = 0; j < 1024; j++) {
			kernel_page_tables[i][j] = page_frame;
			kernel_page_tables[i][j] |= 0x3;
			page_frame += 0x1000;
		}
	}
	return;
}

void setup_pm_gdt(void) {
	pm_gdt_info.len = sizeof(pm_gdt);
	pm_gdt_info.addr = pm_gdt;

	/* make_gdt_entry(gdt_entry*, limit, base, type, flags) */
	/* In x86 (who knows about elsewhere) first GDT entry should be null. */
	make_gdt_entry(&pm_gdt[0], 0x0, 0x0, 0x0, 0x0);
	// KERNEL_CODE_SEGMENT
	// Presently, it is very important that the c kernel code segment be
	// the second entry (offset of 8 bytes from the start.) If you change this,
	// make sure to also change pm_jump() in kernel_entry.asm to use the right
	// segment selector so that CS register is updated correctly.
	make_gdt_entry(&pm_gdt[1], 0xfffff, 0x0, 0xa, 0xc9);
	// KERNEL_DATA_SEGMENT
	make_gdt_entry(&pm_gdt[2], 0xfffff, 0x0, 0x2, 0xc9);
	// USER_CODE_SEGMENT
	make_gdt_entry(&pm_gdt[3], 0x0, 0x0, 0x0, 0x0);
	// USER_DATA_SEGMENT
	make_gdt_entry(&pm_gdt[4], 0x0, 0x0, 0x0, 0x0);

	show_gdt(pm_gdt, 5);
	load_pm_gdt();
}

void show_gdt(struct gdt_entry* gdt, int num_entries) {
	unsigned int limit = 0, base = 0, flags = 0, type = 0;
	int i = 0;

	for (; i < num_entries; i++) {
		limit |= gdt[i].limit0_15;
		limit |= ((gdt[i].limit16_19_avl_l_db_g & 0x0f) << 16);

		base |= gdt[i].base0_15;
		base |= gdt[i].base16_23 << 16;
		base |= gdt[i].base24_31 << 24;

		flags |= (gdt[i].type_s_dpl_p & 0xf0) >> 4;
		flags |= (gdt[i].limit16_19_avl_l_db_g & 0xf0);

		type |= (gdt[i].type_s_dpl_p & 0xf);

		print("limit="); print_int32(limit); print(",");
		print("base="); print_int32(base); print(",");
		print("type="); print_int32(type); print(",");
		print("flags="); print_int32(flags); print("\n");

		(limit = 0, base = 0, flags = 0, type = 0);
	}	
}

extern void pm_jump(void);
void load_pm_gdt(void) {
	asm volatile("lgdt %0" : : "m"(pm_gdt_info));
	pm_jump();
}


void make_gdt_entry(struct gdt_entry* entry,
					unsigned int limit,
					unsigned int base,
					char type,
					/*flags format: S_DPL_P_AVL_L_DB_G*/
					/*bits:         1_2___1_1___1_1__1*/
					char flags) {

	// Set lower 16 bits of limit.
	entry->limit0_15 = limit & 0xffff;
	// Set lower 16 bits of base.
	entry->base0_15 = base & 0xffff;
	// Set bits 16 to 13 of base.
	entry->base16_23 = (base >> 16) & 0xff;
	// Set bits 24 to 31 of base.
	entry->base24_31 = (base >> 24) & 0xff;
	// Set upper 4 bits of 20 bit limit.
	entry->limit16_19_avl_l_db_g = (limit >> 16) & 0xf;
	// Set 4 bits of type.
	entry->type_s_dpl_p = type & 0xf;
	// Set S_DPL_P flags (lower 4 bits of 8 bit flags).
	entry->type_s_dpl_p |= (flags & 0xf) << 4;
	// Set AVL_L_DB_G flags (upper 4 bits of 8 bit flags).
	entry->limit16_19_avl_l_db_g |= flags & 0xf0;
}
