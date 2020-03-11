#include "mm.h"

#include "drivers/screen/screen.h"
#include "string.h"


extern unsigned int mem_map_buf_addr;
extern unsigned int mem_map_buf_entry_count;

extern int _bss_start;
extern int _bss_end;

static uint32_t kernel_mem_bitmap;
static struct bios_mem_map* bmm;

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
}
