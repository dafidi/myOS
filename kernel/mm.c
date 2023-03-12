#include "mm.h"

#include "error.h"
#include "print.h"
#include "string.h"
#include "system.h"
#include "task.h"

// System memory map as told by BIOS.
extern pa_t mem_map_buf_addr;
extern unsigned int mem_map_buf_entry_count;

// Kernel sections' labels. For example, we can get the address of the start of
// the .text section by taking &_text_start
extern const pa_t _bss_start;
extern const pa_t _bss_end;
extern const pa_t _text_start;
extern const pa_t _text_end;
extern const pa_t _data_start;
extern const pa_t _data_end;

// These are set up in init_interrupts.
pa_t _interrupt_stacks_begin = -1;
pa_t _interrupt_stacks_end = -1;
pa_t _interrupt_stacks_length = -1;

// These we can compute from &_{bss, text, data}_{start, end}.
uint64_t _bss_length;
uint64_t _text_length;
uint64_t _data_length;

// TSS for kernel and user tasks.
// Perhaps a future item here is to dynamically allocate user TSSes.
extern tss_t kernel_tss __attribute__((aligned(0x1000)));
extern tss_t user_tss __attribute__((aligned(0x1000)));

// Arbitrarily defined parameters for executing user program.
extern void *APP_START_VIRT_ADDR;
extern void *APP_START_PHY_ADDR;
extern int APP_STACK_SIZE;
extern int APP_HEAP_SIZE;

// These are defined in .asm/.s files.
extern void enable_paging(void);
extern void pm_jump(void);

// Macros for memory management.
#define NUM_BITMAP_INTS (SYSTEM_NUM_PAGES >> 5)

// Memory management structures for the kernel's use.
static uint32_t page_usage_bitmap[NUM_BITMAP_INTS];
static struct bios_mem_map_entry *bmm;

uint64_t _max_available_phy_addr = 0;
uint64_t _available_memory = 0;
uint64_t _zone_designated_memory = 0;

int _highest_initialized_zone_order = 0;

#define NUM_GDT_ENTRIES 8
// Kernel structures for segmentation.
struct gdt_entry pm_gdt[NUM_GDT_ENTRIES];
struct gdt_info pm_gdt_info = {
    .len = sizeof(pm_gdt),
    .addr = (unsigned long)pm_gdt,
};

// Kernel structures for paging.
// We have 1024 page directory entries -> 1024 page tables and each page table has 1024 ptes.
unsigned int kernel_page_directory[1024]__attribute__((aligned(0x1000)));
static unsigned int kernel_page_tables[1024][1024]__attribute__((aligned(0x1000)));

// User structures for paging.
unsigned int user_page_directory[USER_PAGE_DIR_SIZE]__attribute__((aligned(0x1000)));
static unsigned int user_page_tables[USER_PAGE_DIR_SIZE][USER_PAGE_TABLE_SIZE]__attribute__((aligned(0x1000)));

/**
 * An array of lists of mem_blocks. Each list of mem_blocks contains
 * mem_blocks of the same order. I.e. order_zones[i] is a list
 * of mem_blocks of size 2^i.
 * 
 * "MAX_ORDER + 1" because we want to be able to say "order_zones[MAX_ORDER]".
 */
struct order_zone order_zones[MAX_ORDER + 1];
struct mem_block mem_block_pool[SYSTEM_NUM_PAGES];

struct memory_object_cache memory_object_caches[MEMORY_OBJECT_ORDER_RANGE + 1]; // +1 is for  NULL-termination

/**
 * map_va_range_to_pa_range - Map a contiguos block of virtual memory to a contiguous block of
 * physical memory.
 * 
 * @page_tables_ptr: A list of page tables where mapping are stored.
 * @va: The base address of the block of virtual memory to be mapped.
 * @range_size_bytes: The length of the block of virtual memory to be mapped.
 * @pa: The base address of the block of physical memory to be mapped to.
 */
static int map_va_range_to_pa_range(unsigned int page_tables_ptr[][USER_PAGE_TABLE_SIZE],
                             va_t va, va_range_sz_t range_size_bytes, unsigned int pa,
                             unsigned int access_flags) {
    unsigned int va_aligned = PAGE_ALIGN(va), pa_aligned = PAGE_ALIGN(pa);
    unsigned int i_start, j_start;
    unsigned int i_end, j_end;
    unsigned int i, j;

    i_end = (va_aligned + range_size_bytes) / (USER_PAGE_TABLE_SIZE * PAGE_SIZE);
    j_end = ((va_aligned + range_size_bytes) / PAGE_SIZE) % USER_PAGE_TABLE_SIZE;
    i_start = va_aligned / (USER_PAGE_TABLE_SIZE * PAGE_SIZE);
    j_start = (va_aligned / PAGE_SIZE) % USER_PAGE_TABLE_SIZE;
    i = i_start, j = j_start;

    if (j_end)
        i_end++;

    for (; i < i_end; i++) {
        int tmp_j_end = USER_PAGE_TABLE_SIZE;

        if (i == i_end - 1)
            tmp_j_end = j_end;

        for (; j < tmp_j_end; j++) {
            page_tables_ptr[i][j] = pa_aligned | access_flags;
            pa_aligned += PAGE_SIZE;
        }
        j = 0;
    }
    return 0;
}

static int unmap_va_range_to_pa_range(unsigned int page_tables_ptr[][USER_PAGE_TABLE_SIZE],
                             va_t va, va_range_sz_t range_size_bytes, unsigned int pa,
                             unsigned int access_flags) {
    unsigned int va_aligned = PAGE_ALIGN(va);
    unsigned int i_start, j_start;
    unsigned int i_end, j_end;
    unsigned int i, j;

    i_end = (va_aligned + range_size_bytes) / (USER_PAGE_TABLE_SIZE * PAGE_SIZE);
    j_end = ((va_aligned + range_size_bytes) / PAGE_SIZE) % USER_PAGE_TABLE_SIZE;
    i_start = va_aligned / (USER_PAGE_TABLE_SIZE * PAGE_SIZE);
    j_start = (va_aligned / PAGE_SIZE) % USER_PAGE_TABLE_SIZE;
    i = i_start, j = j_start;

    if (j_end)
        i_end++;

    for (; i < i_end; i++) {
        int tmp_j_end = USER_PAGE_TABLE_SIZE;

        if (i == i_end - 1)
            tmp_j_end = j_end;

        for (; j < tmp_j_end; j++)
            page_tables_ptr[i][j] = 0x0;

        j = 0;
    }
    return 0;
}

void mark_page_used(const int page_idx) {
    set_bit((uint8_t *)page_usage_bitmap, page_idx);
}

void mark_page_unused(const int page_idx) {
    clear_bit((uint8_t *)page_usage_bitmap, page_idx);
}

unsigned int get_available_memory(void) {
    unsigned long long free_memory = 0;
    int i = 0;

    for (i = 0; i < SYSTEM_NUM_PAGES; i++) {
        if (get_bit((uint8_t *)page_usage_bitmap, i) == 0)
            free_memory += PAGE_SIZE;
    }

    return free_memory;
}

int get_next_free_user_page(void) {
    return -1;
}

int reserve_and_map_user_memory(va_t va, pa_t pa, unsigned int amount) {
    int num_pages;
    int page_idx;
    int i;

    // Verify requested physical address is not in [0, _bss_end)
    if (pa < (pa_t)&_bss_end)
        return -1;
    page_idx = pa >> 12;

    // Our arbitrary policy is that programs should expect to start at
    // addresses no less than 250MB.
    if (va < 0x10000000U)
        return -1;

    if (amount > 3 * 0x40000000U)
        return -1;
    num_pages = amount / PAGE_SIZE;

    for (i = page_idx; i < page_idx + num_pages; i++)
        mark_page_used(i);

    map_va_range_to_pa_range(user_page_tables, va, (va_range_sz_t)amount, pa, 0x7);

    return 0;
}

int unreserve_and_unmap_user_memory(va_t va, pa_t pa, unsigned int amount) {
    int num_pages;
    int page_idx;
    int i;

    num_pages = amount / PAGE_SIZE;
    page_idx = pa >> 12;

    for (i = page_idx; i < page_idx + num_pages; i++)
        mark_page_unused(i);

    unmap_va_range_to_pa_range(user_page_tables, va, (va_range_sz_t)amount, pa, 0x7);

    return 0;
}

/**
 * @brief Set up kernel & user page directory/tables.
 * 
 * What it does currently:
 * 
 * 1. Identity map all 4G of RAM for kernel page tables & directory.
 * 
 * 2.  (i) Identity map kernel-occupied region into  user page tables & directory.
 * 	  (ii) Arbitrarily map user-program-occupied memory into user page-tables and directory.
 * 		   The current mapping is very simple: VA [0x30000000, ...) => PA [0x20000000, ...).
 */
static void setup_page_directory_and_page_tables(void) {
    va_range_sz_t region_length;
    unsigned int i, page_frame;
    va_t region_start;
    pa_t addr;

    print_string("kernel_page_directory="); 	print_int32((pa_t) kernel_page_directory); 	print_string("\n");
    print_string("user_page_directory="); 		print_int32((pa_t) user_page_directory); 	print_string("\n");
    print_string("kernel_page_tables="); 		print_int32((pa_t) kernel_page_tables); 	print_string("\n");
    print_string("user_page_tables="); 			print_int32((pa_t) user_page_tables); 		print_string("\n");

    // Setup kernel paging structures.
    addr = (pa_t) &kernel_page_tables[0];
    for (i = 0; i < 1024; i++) {
        // Set all PDEs.
        kernel_page_directory[i] = (addr & 0xfffff000) | 0x3;
        addr += 0x1000;
    }

    {
        region_length = (va_range_sz_t)0x100000000ULL;
        region_start = 0x0;
        page_frame = 0x0;
        map_va_range_to_pa_range(kernel_page_tables, /*va=*/(va_t)region_start, /*size=*/region_length, /*pa=*/page_frame,
                                 /*access_flags=*/0x3);
    }

    // Setup user paging structures. These generally should not change across user programs.
    // We'll set up the mapping of the program itself when the program is about to be run.
    addr = (pa_t) &user_page_tables[0];
    for (i = 0; i < USER_PAGE_DIR_SIZE; i++) {
        // Set all PDEs.
        user_page_directory[i] = (addr & 0xfffff000) | 0x7;
        addr += 0x1000;
    }

    {
        /**
         * Identity map the kernel into the process's virtual address space.
         * There are 2 usable portions of physical memory:
         * 		1. [0x0, 0x9fc00)
         * 		2. [0x100000, 0xbffe0000)
         * We are arbitrarily deciding that only the kernel is allowed in the first part.
         * We map this portion of the kernel here.
         * VA: 0x0 -> PAGE_ALIGN(0x9fc00), PA: 0x0 -> PAGE_ALIGN(0x9fc00)
         */
        region_length = (unsigned int)0x9fc00;
        region_start = 0x0;
        page_frame = 0x0;
        map_va_range_to_pa_range(user_page_tables, /*va=*/(va_t)region_start, /*size=*/region_length, /*pa=*/page_frame,
                                 /*access_flags=*/0x1);
    }

    {
        /**
         * We map the part of the kernel in the second portion of usable physical memory here.
         * We know that the kernel uses only [0x100000, _bss_end) because that is what is specified in kernel.ld :).
         * Should that change, this should probably change too.
         * We must use "PAGE_ALIGN_UP(&_bss_end)" here and not just "&_bss_end" as the latter would neglect
         * memory between.
         * [PAGE_ROUND_DOWN(&_bss_end), PAGE_ROUND_DOWN(&_bss_end) + PAGE_SIZE).
         */
        region_length = PAGE_ALIGN_UP((pa_t)&_bss_end) - 0x100000;
        region_start = 0x100000;
        page_frame = 0x100000;
        map_va_range_to_pa_range(user_page_tables, /*va=*/(va_t)region_start, /*size=*/region_length, /*pa=*/page_frame,
                                 /*access_flags=*/0x1);
    }
}

/**
 * @brief Configure, setup and enable paging. 
 */
void setup_and_enable_paging(void) {
    setup_page_directory_and_page_tables();
    enable_paging();
}

/**
 * @brief Load global variable pm_gdt_info into the processor's GDTR.
 */
static void load_pm_gdt(void) {
#ifdef CONFIG32
    asm volatile("lgdt %0" : : "m"(pm_gdt_info));
    pm_jump();
#endif
}

/**
 * @brief Display the first num_entries of the provided gdt.
 * 
 * @gdt: pointer to array of GDT entries.
 * @num_entries: the number of GDT entries to be display.
 */
static void show_gdt(struct gdt_entry* gdt, int num_entries) {
    unsigned int limit = 0, base = 0, flags = 0, type = 0;
    int i;

    for (i = 0; i < num_entries; i++) {
        limit |= ((gdt[i].limit16_19_avl_l_db_g & 0x0f) << 16);
        limit |= gdt[i].limit0_15;

        base |= gdt[i].base24_31 << 24;
        base |= gdt[i].base16_23 << 16;
        base |= gdt[i].base0_15;

        flags |= (gdt[i].limit16_19_avl_l_db_g & 0xf0);
        flags |= (gdt[i].type_s_dpl_p & 0xf0) >> 4;

        type |= (gdt[i].type_s_dpl_p & 0xf);

        print_string("limit="); print_int32(limit); print_string(",");
        print_string("base="); 	print_int32(base); 	print_string(",");
        print_string("type="); 	print_int32(type); 	print_string(",");
        print_string("flags="); print_int32(flags); print_string("\n");

        (limit = 0, base = 0, flags = 0, type = 0);
    }	
}

/**
 * @brief Set the bits of the gdt entry provided with the values provided.
 * This follows the ia32 specification.
 * 
 * @entry: pointer to the gdt_entry to the configured.
 * @limit: the segments size.
 * @base:  the starting address of the segment.
 * @type:  type as specifies by ia32 specs.
 * @flags: flags are specified by ia32 specs.
 */
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

void make_gdt64_tss_entry(struct gdt64_tss_entry* entry,
                    unsigned int limit,
                    uint64_t base,
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
    entry->type_s_dpl_p |= (flags & 0xe) << 4;
    // Set AVL_L_DB_G flags (upper 4 bits of 8 bit flags).
    entry->limit16_19_avl_l_db_g |= flags & 0x90;
    entry->base32_63 = base & 0xffffffff00000000ULL;
}

/**
 * @brief Configure and update system GDT.
 * The current configuration defines the following GDT configuration:
 * 
 * 0: NULL SEGMENT - required (to be the first entry) per ia32 specs.
 * 1: KERNEL CODE SEGMENT DESCRIPTOR
 * 2: KERNEL DATA SEGMENT DESCRIPTOR
 * 3: USER CODE SEGMENT DESCRIPTOR
 * 4: USER DATA SEGMENT DESCRIPTOR
 * 5: KERNEL TASK SEGMENT DESCRIPTOR
 * 6: USER TASK SEGMENT DESCRIPTOR
 */
void setup_and_load_pm_gdt(void) {
    pm_gdt_info.addr = (long unsigned int) pm_gdt;
    pm_gdt_info.len = sizeof(pm_gdt);

    /* make_gdt_entry(gdt_entry*, limit, base, type, flags) */
    /* In x86 (who knows about elsewhere) first GDT entry should be null. */
    make_gdt_entry(&pm_gdt[0], 0x0, 0x0, 0x0, 0x0);
    // KERNEL_CODE_SEGMENT
    make_gdt_entry(&pm_gdt[KERNEL_CODE_SEGMENT_IDX], 0xfffff, 0x0, 0xa, 0xc9);
    // KERNEL_DATA_SEGMENT
    make_gdt_entry(&pm_gdt[KERNEL_DATA_SEGMENT_IDX], 0xfffff, 0x0, 0x2, 0xc9);
    // USER_CODE_SEGMENT
    make_gdt_entry(&pm_gdt[USER_CODE_SEGMENT_IDX], 0xfffff, 0x0, 0xa, 0xcf);
    // USER_DATA_SEGMENT
    make_gdt_entry(&pm_gdt[USER_DATA_SEGMENT_IDX], 0xfffff, 0x0, 0x2, 0xcf);

    // Set kernel and user TSS descriptors to zero - the task system will
    // initialize them.
    make_gdt_entry(&pm_gdt[KERNEL_TSS_DESCRIPTOR_IDX], 0x0, 0x0, 0x0, 0x0);
    make_gdt_entry(&pm_gdt[USER_TSS_DESCRIPTOR_IDX], 0x0, 0x0, 0x0, 0x0);

    show_gdt(pm_gdt, NUM_GDT_ENTRIES - 1);
    load_pm_gdt();
}

/**
 * @brief Initialize page usage bitmap.
 */
void init_page_usage_bitmap(void) {
    unsigned long last_page_idx = ((unsigned long) &_bss_end) >> 12;
    int num_pages = last_page_idx + 1;

    // Initialize bitmap to zero.
    fill_long_buffer((unsigned int *)page_usage_bitmap, 0,  NUM_BITMAP_INTS, 0x0);

    // Set every page up to _bss_end as being used.
    for (int i = 0; i < num_pages; i++)
        set_bit((uint8_t *)page_usage_bitmap, i);
}

static bool zone_state_set(struct order_zone *zone, enum order_zone_state state) {
	return zone->state & state;
}

static void set_zone_state(struct order_zone *zone, enum order_zone_state state) {
    zone->state |= state;
}

static void unset_zone_state(struct order_zone *zone, enum order_zone_state state) {
	zone->state &= ~state;
}

static void mark_zone_initialized(struct order_zone *zone) {
    set_zone_state(zone, INITIALIZED);
}

static void mark_zone_uninitialized(struct order_zone *zone) {
    unset_zone_state(zone, INITIALIZED);
}

static bool zone_initialized(struct order_zone *zone) {
	return zone_state_set(zone, INITIALIZED);
}

/**
 * @brief Add block to the end of zone's used_list.
 * 
 * @param zone 
 * @param block 
 */
static void zone_append_used(struct order_zone *zone, struct mem_block *block) {
    struct mem_block *used_tail = zone->used_list;

    if (used_tail) {
        while (used_tail->next)
            used_tail = used_tail->next;
        used_tail->next = block;
    } else {
        zone->used_list = block;
    }

    block->next = NULL;
    block->state = USED;
    block->order = zone->order;

    zone->used++;
}

/**
 * @brief Add block to the end of zone's free_list.
 * 
 * @param zone 
 * @param block 
 */
static void zone_append_free(struct order_zone *zone, struct mem_block *block) {
    struct mem_block *free_tail = zone->free_list;

    if (free_tail) {
        while (free_tail->next)
            free_tail = free_tail->next;
        free_tail->next = block;
    } else {
        zone->free_list = block;
    }

    block->next = NULL;
    block->state = FREE;
    block->order = zone->order;

    zone->free++;
}

/**
 * @brief Add block to the beginning of zone's used_list.
 * 
 * @param zone 
 * @param block 
 */
/* Commenting this out for now as it's not being used generates a warning.
static void zone_prepend_used(struct order_zone *zone, struct mem_block *block) {
    block->next = zone->used_list;
    block->order = zone->order;
    block->state = USED;

    zone->used_list = block;
    zone->used++;
}
*/

/**
 * @brief Add block to the end of zone's free_list.
 * 
 * @param zone 
 * @param block 
 */
static void zone_prepend_free(struct order_zone *zone, struct mem_block *block) {
    block->next = zone->free_list;
    block->order = zone->order;
    block->state = FREE;

    zone->free_list = block;
    zone->free++;
}

/**
 * @brief Remove block from zone's used_list.
 * 
 * @param zone 
 * @param block 
 */
static void zone_remove_used(struct order_zone* zone, struct mem_block *block) {
    struct mem_block *predecessor;
        
    if (!zone->used_list) {
        print_string("The zone's used_list is empty yet we are tring to free block=");
        print_ptr(block); print_string(", which is BAD.\n");
        return;
    }

    if (zone->used_list == block) {
        zone->used_list = zone->used_list->next;
        goto done;
    }

    predecessor = zone->used_list;
    while(predecessor->next != block) {
        predecessor = predecessor->next;
        if (!predecessor) {
            print_string("Could not find block="); print_ptr(block);
            print_string(" in the used_list of zone"); print_int32(zone->order);
            print_string(" which is BAD.\n");
            return;
        }
    }

    predecessor->next = block->next;

done:
    zone->used--;
}

/**
 * @brief Remove block from zone's free_list.
 * 
 * @param zone 
 * @param block 
 */
static void zone_remove_free(struct order_zone* zone, struct mem_block *block) {
    struct mem_block *predecessor;
        
    if (!zone->free_list) {
        print_string("The zone's used_list is empty yet we are tring to free block=");
        print_ptr(block); print_string(", which is BAD.\n");
        return;
    }

    if (zone->free_list == block) {
        zone->free_list = zone->free_list->next;
        goto done;
    }

    predecessor = zone->free_list;
    while(predecessor->next != block) {
        predecessor = predecessor->next;
        if (!predecessor) {
            print_string("Could not find block="); print_ptr(block);
            print_string(" in the used_list of zone "); print_int32(zone->order);
            print_string(" which is BAD.\n");
            return;
        }
    }

    predecessor->next = block->next;

done:
    zone->free--;
}

/**
 * @brief Get the buddy of a block in the context of a zone.
 * 
 * Since a zone has 2^zone->order pages in a block, the buddy of a given block
 * , "in the context of a zone", is 2^zone->order mem_blocks away from the
 * block.
 * 
 * @param zone 
 * @param block 
 * @return struct mem_block* 
 */
static struct mem_block *get_block_buddy(struct order_zone *zone, struct mem_block *block) {
    /* 
        Pointer arithmetic means this gives the index in terms of blocks rather
        than byte offset.
    */
    uint32_t block_pool_index = block - &mem_block_pool[0];

    /*
    This could be a source of brutal headaches, I'll leave this here to get
    debug prints going quickly.

    print_string("block="); print_int32(block);
    print_string(",&mem_block_pool[0]="); print_int32(&mem_block_pool[0]);
    print_string(",block_pool_index="); print_int32(block_pool_index); print_string("\n");
    */

    /*
      I'm not really able to think of the specific reason but I just know
      that if a block is at an odd index from the start of the pool, it must
      be the RHS of the pair of buddies, so its buddy would be on the left,
      requiring subtraction here.
      Also, 2^zone->order is added or subtracted because a block's buddy is that
      many blocks away from it.
    */
    if (block_pool_index & 0x1)
        return block - (1 << zone->order);
    return block + (1 << zone->order);
}

/**
 * @brief Extract the head of the zone's list of FREE blocks,
 * split it into 2 halves and return them. The blocks are going into the
 * next-lower-order zone (NLOZ).
 * 
 * @param zone
 * @param block
 * @param block_buddy
 */
static void zone_split_head(struct order_zone *zone,
                     /*out*/struct mem_block **block,
                     /*out*/struct mem_block **block_buddy) {
    struct mem_block *old_freelist_head, *old_freelist_head_buddy;
    unsigned int num_pages_in_split_blocks;
    struct order_zone *nloz;
    uint8_t nlo;

    old_freelist_head = zone->free_list;
    zone_remove_free(zone, old_freelist_head);

    nlo = zone->order - 1;
    old_freelist_head->order = nlo;
    
    num_pages_in_split_blocks = 1 << (nlo);
    nloz = &order_zones[nlo];

    // Get the buddy of old_free_list_head
    old_freelist_head_buddy = get_block_buddy(nloz, old_freelist_head);
    if (old_freelist_head_buddy < old_freelist_head)
        old_freelist_head_buddy->addr = old_freelist_head->addr - (PAGE_SIZE * num_pages_in_split_blocks);
    else
        old_freelist_head_buddy->addr = old_freelist_head->addr + (PAGE_SIZE * num_pages_in_split_blocks);
    old_freelist_head_buddy->order = nlo;
    old_freelist_head_buddy->trueorder = old_freelist_head->trueorder;

    *block = old_freelist_head;
    *block_buddy = old_freelist_head_buddy;	
}

/**
 * @brief Try to request that zone splits one of it's FREE blocks into 2 and
 * pass these back in block and block_buddy.
 * 
 * @param zone 
 * @param block 
 * @param block_buddy 
 * @return uint32_t 
 */
static uint32_t zone_borrow(struct order_zone *zone,
                            /*out*/ struct mem_block **block,
                            /*out*/ struct mem_block **block_buddy) {
    uint32_t error = 0;

    // If the zone's free_list is empty we need to borrow form the next higher
    // order zone (NHOZ).
    if (!zone->free_list) {
        struct mem_block *to_return, *to_keep;
        uint8_t nho = zone->order + 1;
        struct order_zone *nhoz;
        
        if (nho > _highest_initialized_zone_order)
            return -1;

        // Get the zone from which we'll request blocks;
        nhoz = &order_zones[nho];

        error = zone_borrow(nhoz, &to_return, &to_keep);
        if (error)
            return -1;

        zone_append_free(zone, to_keep);

        // Insert the new block at the head of the free list so that
        // it can be split and returned by zone_split_head;
        zone_prepend_free(zone, to_return);
    }

    zone_split_head(zone, block, block_buddy);

    return error;
}

/**
 * @brief Re-integrate a block that was previously borrowed by a lower-order zone.
 * 
 * @param zone 
 * @param block 
 */
static void zone_collect(struct order_zone *zone, struct mem_block *block) {
    if (zone->order == block->trueorder) {
        zone_append_free(zone, block);
    } else {
        uint8_t nho = zone->order + 1;
        struct mem_block *buddy;
        struct order_zone *nhoz;

        nhoz = &order_zones[nho];
        buddy = get_block_buddy(zone, block);
        if (buddy->state == USED) {
            zone_append_free(zone, block);
        } else {
            zone_remove_free(zone, buddy);
            // Tell NHOZ to add this block.
            zone_collect(nhoz, block);
        }
    }
}

/**
 * @brief Allocate a block from a zone.
 * 
 * @param zone 
 * @return struct mem_block* 
 */
struct mem_block *__zone_alloc(struct order_zone *zone) {
    struct mem_block *block = NULL;
    /**
     * Algorithm:
     * 
     * Try to get a block from the free_list:
     * 		If there is a free block:
     * 			mark it "USED"
     * 			move it from the free_list to the used_list.
     * 			return it
     * 		Else:
     * 			Figure out the next higher order zone (NHOZ) order_zones[order+1].
     * 			If it is <= MAX_ORDER:
     * 				Try to request that the NHOZ split one if it's freeblocks and give them to us: //<--BIG Q: is this recursive?----------------------------->//
     * 					If this succeeds:														   //<--Also note that the blocks' "order"s must be changed.-->//
     * 						Add one block to the free_list, add the other to the used_list and return the other.
     * 					Else:
     * 						return NULL.
     */
    if (zone->free_list) {
        block = zone->free_list;
        zone_remove_free(zone, block);
        zone_append_used(zone, block);
    } else {
        // Try to get 2 blocks from the next higher order zone (NHOZ).
        struct mem_block *borrowed_block, *borrowed_block_buddy;
        struct order_zone *nhoz;
        uint32_t error;
        uint8_t nho;

        nho = zone->order + 1;
        if (nho > _highest_initialized_zone_order)
            goto done;
    
        nhoz = &order_zones[nho];
        error = zone_borrow(nhoz, &borrowed_block, &borrowed_block_buddy);
        if (error)
            return NULL;

        zone_append_free(zone, borrowed_block_buddy);
        zone_append_used(zone, borrowed_block);
        block = borrowed_block;
    }

done:
    return block;
}

struct mem_block *zone_alloc(const int amt) {
    int order = 0;

    while (amt > ORDER_SIZE(order))
        order += 1;

    if (order > _highest_initialized_zone_order)
        return NULL;

    return __zone_alloc(&order_zones[order]);
}

/**
 * @brief Free a block from a zone.
 * 
 * @param zone 
 * @param block
 * @return struct mem_block* 
 */
void __zone_free(struct order_zone *zone, struct mem_block *block) {
    struct mem_block *buddy;
    /**
     * Algorithm:
     * 
     * If block->trueorder == block->order:
     * 		Simply mark the block "FREE".
     * 		Move it from the current zone's used_list to its free_list.
     * Else:
     * 		If this block's buddy is not free:
     * 			Simply mark the block "FREE".
     * 			Move it from the current zone's used_list to its free_list.
     * 			//<--We can only combine them when they are both free.-->//
     * 			//<--We'll try to do so later.-------------------------->//
     * 		Else:
     * 			(Using the left block's mem_block pointer, try to return the
     * 			block to order_zones[order+1], NHOZ)
     *			Remove this block from the current zone's used_list. //<--Note that it may not be present in the used_list due to the recursive nature of this free function and that's OK.-->//
     * 			Set block->order = block->order + 1.
     * 			Recursively call zone_free(NHOZ, BLOCK)
     * 			
     */
    if (block->trueorder == zone->order) {
        zone_remove_used(zone, block);
        zone_append_free(zone, block);
        return;
    }

    // block->trueorder != block->order ... the interesting stuff!
    // Fetch the block's buddy.
    buddy = get_block_buddy(zone, block);
    zone_remove_used(zone, block);
    if (buddy->state == USED) {
        // Remove the block from the used list and put it in the free list.
        zone_append_free(zone, block);
    } else { // Ah, the REALLY interesting stuff!
        uint8_t nho = zone->order + 1;
        struct order_zone *nhoz;
        
        zone_remove_free(zone, buddy);
        // Tell NHOZ to add this block.
        nhoz = &order_zones[nho];
        zone_collect(nhoz, block);
    }
}

void zone_free(struct mem_block *block) {
    struct order_zone *zone = &order_zones[block->order];

    __zone_free(zone, block);
}

void static print_order_zone(const struct order_zone *const zone) {
    void *pmem_start = (void *) u64_to_addr(zone->phy_mem_start);
    const int num_block_pages = 1 << (zone->order);

    print_string("Zone: "); 			print_int32(zone->order);
    print_string(" free_list: ");	    print_ptr(zone->free_list);
    print_string(" free_list->next: ");	print_ptr(zone->free_list->next);
    print_string(" blocks: ");			print_int32(zone->num_blocks);
    print_string(" pages_per_block: ");	print_int32(num_block_pages);
    print_string(" total_size: ");		print_int64((uint64_t)num_block_pages * PAGE_SIZE * zone->num_blocks);
    print_string(" addr: ");
    // For some reason, gcc uses:
    //     mov $eax, $edi
    //     callq print_int64
    // in most cases when calling print_int64 and it's not clear why.
    // This is a problem because we don't get the upper 4 bytes of the param.
    // print_ptr seems to set up the param properly using $rdi so we're using
    // that as  a workaround for now.
    print_ptr(pmem_start);
    print_string(" state: "); print_int32(zone->state);
    print_string("\n");
}

/**
 * @brief Initialize the order_zone structure for a given order.
 * 
 * @param order 
 * @param phy_mem_start: Physical address where the zone starts.
 * @param zone_size: Size of the zone in bytes.
 */
void init_order_zone(const uint8_t order, const pa_t phy_mem_start, const pa_range_sz_t zone_size) {
    const int num_blocks = zone_size >> (PAGE_SIZE_SHIFT + order);
    struct order_zone *const zone = &order_zones[order];
    int block_index = 0, mem_block_pool_index = 0;
    const int num_block_pages = 1 << order;

    if (num_blocks == 0) {
        print_string("zone "); print_int32(order); print_string(" cannot be initialized.\n");
        mark_zone_uninitialized(zone);
        return;
    }

    /* Initialize the zone. */
    zone->free_list = &mem_block_pool[order * DEFAULT_PAGES_PER_ZONE];
    zone->num_blocks = num_blocks;
    zone->used_list = NULL;
    zone->used = 0;
    zone->free = 0;
    zone->order = order;
    zone->phy_mem_start = phy_mem_start;

    /**
     * To index zone->free_list, we must skip num_block_pages mem_blocks as
     * each block spans that many pages. This makes finding a block's buddy and
     * its corresponding physical address straightforward (see
     * get_block_buddy()).
     */
    for (; block_index < num_blocks; block_index += 1, mem_block_pool_index += num_block_pages) {
        zone->free_list[mem_block_pool_index].addr = phy_mem_start + (num_block_pages * PAGE_SIZE * block_index);
        zone->free_list[mem_block_pool_index].next = &zone->free_list[mem_block_pool_index + num_block_pages];
        zone->free_list[mem_block_pool_index].trueorder = order;
        zone->free_list[mem_block_pool_index].order = order;
        zone->free_list[mem_block_pool_index].state = FREE;
        zone->free++;
    }

    zone->free_list[mem_block_pool_index - num_block_pages].next = NULL;
    mark_zone_initialized(zone);

    /* It's nice to see the initialized zone's description. */
    print_order_zone(zone);
}

bool starts_in_region(struct bios_mem_map_entry *region_entry, pa_t raw_addr) {
    uint64_t start, end;

    start = region_entry->base;
    end = start + region_entry->length;

    if (start <= raw_addr && raw_addr < end)
        return true;

    return false;
}

bool fits_in_region(struct bios_mem_map_entry *region_entry, pa_t raw_addr, pa_range_sz_t size) {
    uint64_t start, end;

    start = region_entry->base;
    end = start + region_entry->length;

    if (start <= raw_addr && (raw_addr + size) < end)
        return true;

    return false;
}

bool region_is_available(struct bios_mem_map_entry *region_entry) {
    return region_entry->type == 1;
}

uint64_t next_region_addr(int i) {
    uint64_t addr = 0;

    if (i < mem_map_buf_entry_count - 1)
        addr = bmm[i + 1].base;

    return addr;
}

pa_t next_available_start(pa_t raw_addr, pa_range_sz_t *size, pa_range_sz_t *next_size) {
    for (int i = 0; i < mem_map_buf_entry_count; i++) {
        struct bios_mem_map_entry *region_entry = &bmm[i];

        if (raw_addr < region_entry->base)
            raw_addr = region_entry->base;

        if (starts_in_region(region_entry, raw_addr)) {
            if (fits_in_region(region_entry, raw_addr, *size) && region_is_available(region_entry)) {
                uint64_t gift_limit = region_entry->base + region_entry->length;

                if (gift_limit > _max_available_phy_addr)
                    gift_limit = _max_available_phy_addr;
                // If the next allocation won't fit into the region used for the current designation,
                // gift the current designation the remaining room in the current region.
                if (raw_addr + *size + *next_size > gift_limit)
                    *size += gift_limit - (raw_addr + *size);
                return raw_addr;
            } else {
                raw_addr = next_region_addr(i);
            }
        }
    }

    return 0;
}

/**
 * @brief Setup page allocator.
 */
static void setup_zone_alloc_free(void) {
    unsigned long long dynamic_memory_start =
        PAGE_ALIGN_UP((pa_t)_interrupt_stacks_end);
    /* TODO: Use dynamically-gotten RAM size instead of hard-coded 4GiB. */

    /* We need to somehow split the memory region [_bss_end, 4GiB)		   */
    /* among the orders. Meaning: how much memory do we split up into size */
    /*     PAGE_SIZE(2^0 *PAGE_SIZE)? 									   */
    /* 2 * PAGE_SIZE(2^1 *PAGE_SIZE)? 									   */
    /* 4 * PAGE_SIZE(2^2 *PAGE_SIZE)?									   */
    /* etc?																   */

    /* Perhaps it is reasonable to assume that smaller allocations are     */
    /* needed more frequently and larger ones less so.					   */
    /* Let's see what happens when we divide memory among the orders 	   */
    /* equally. Of course, since larger mem_blocks are ... larger, there'll*/
    /* fewer entries (mem_blocks) in chains with larger mem_blocks.		   */
    /* Let's call the regions of memory divided into a given order an	   */
    /* "order zone."													   */
    /*                                                                     */
    /* Also, we don't seem to be able to write to the last 1GB of physical */
    /* memory so let's avoid that regions until it is better understood.   */
    const uint64_t designatable_memory = _available_memory;
#ifndef CONFIG32
    const pa_range_sz_t order_zone_size = designatable_memory / (MAX_ORDER + 1);
#else
    const pa_range_sz_t order_zone_size = udiv(designatable_memory, (uint64_t) (MAX_ORDER + 1));
#endif
    pa_range_sz_t next_size = order_zone_size;
    pa_t start_addr = dynamic_memory_start;
    pa_range_sz_t size;

    for (int i = 0; i <= MAX_ORDER; i++, start_addr += size) {
        pa_t available_start;

        size = order_zone_size;

        // For the last zone, make the next_size so big that the zone will be gifted
        // whatever memory is left in the region it is assigned.
        if (i == MAX_ORDER)
            next_size = 0xffffffff;

        available_start = next_available_start(start_addr, &size, &next_size);
        if (available_start == 0ULL) {
            print_string("[Error] no more available memory at order="); print_int32(i); print_string("\n");
            print_string("request:");
            print_string("addr="); print_int64(start_addr); print_string(",");
            print_string("size="); print_int64(size); print_string("\n");
            return;
        }

        init_order_zone(/*order=*/i,
                        /*phy_mem_start=*/available_start,
                        /*zone_size=*/size);

        struct order_zone *const zone = &order_zones[i];

        if (zone_initialized(zone)) {
            _zone_designated_memory += (1 << i) * PAGE_SIZE * zone->num_blocks;
            _highest_initialized_zone_order = i;
        }

        start_addr = available_start;
    }
}

/**
 * @brief Insert new object at the head of free list.
 *
 * @param cache
 * @param new_mo
 */
static void object_prepend_free(struct memory_object_cache *cache, struct memory_object *new_mo) {
    new_mo->header.next = cache->free_objects;
    cache->free_objects = new_mo;
    cache->free++;
}

/**
 * @brief Insert new object at the head of used list.
 *
 * @param cache
 * @param new_mo
 */
static void object_prepend_used(struct memory_object_cache *cache, struct memory_object *new_mo) {
    new_mo->header.next = cache->used_objects;
    cache->used_objects = new_mo;
    cache->used++;
}

/**
 * @brief Extract object at the head of free list. For removal
 * from the free list, this is okay as any free object would work
 * - so just do the simple thing and take the first one.
 *
 * @param cache
 * @return struct memory_object*
 */
static struct memory_object *object_remove_free(struct memory_object_cache *cache) {
    struct memory_object *mo = cache->free_objects;

    if (!mo)
        return NULL;

    cache->free_objects = cache->free_objects->header.next;
    cache->free--;

    mo->header.next = NULL;

    return mo;
}

/**
 * @brief Extract object from the list of used objects. Here, unlike with the free list,
 * we are extracting a specific object so we need to traverse the list.
 *
 * @param cache
 * @param addr
 * @return struct memory_object*
 */
static struct memory_object *object_remove_used(struct memory_object_cache *cache, uint8_t *addr) {
    struct memory_object *mo = cache->used_objects;
    struct memory_object *prev_mo;
    uint8_t *mo_addr;

    if (!mo)
        return NULL;

    mo_addr = (uint8_t *)(cache->used_objects) + sizeof(struct memory_object_header);
    if (mo_addr == addr) {
        cache->used_objects = cache->used_objects->header.next;
        cache->used--;

        mo->header.next = NULL;

        return mo;
    }

    prev_mo = mo;
    mo = prev_mo->header.next;
    mo_addr = (uint8_t *)(mo) + sizeof(struct memory_object_header);
    while (mo && mo_addr != addr) {
        prev_mo = mo;
        mo = mo->header.next;
        mo_addr = (uint8_t *)(mo) + sizeof(struct memory_object_header);
    }

    if (!mo) {
        print_string("cache-"); print_int32(cache->order);
        print_string(" has no used mo for addr="); print_int32((pa_t)addr);
        print_string("\n");
        return NULL;
    }

    prev_mo->header.next = mo->header.next;
    cache->used--;

    mo->header.next = NULL;

    return mo;
}

/**
 * @brief Allocate a free object from a given cache.
 *
 * @param cache
 * @return uint8_t*
 */
static uint8_t *__object_alloc(struct memory_object_cache* cache) {
    struct memory_object* mo;

    if (!cache->free_objects)
        return NULL;

    mo = object_remove_free(cache);
    if (!mo)
        return NULL;

    object_prepend_used(cache, mo);
    return (uint8_t *)(mo) + sizeof(struct memory_object_header);
}

/**
 * @brief Allocate a memory object of size greater than or equal to sz.
 *
 * @param sz
 * @return uint8_t*
 */
uint8_t *object_alloc(int sz) {
    struct memory_object_cache *cache;
    int order = MIN_MEMORY_OBJECT_ORDER;

    while ((1 << order) < sz)
        order++;

    if (order > MAX_MEMORY_OBJECT_ORDER)
        return NULL;

    cache = &memory_object_caches[order - MIN_MEMORY_OBJECT_ORDER];

    return __object_alloc(cache);
}

/**
 * @brief Get the header for a memory object. The header immediately precedes
 * the body/actual object and contains metadata about the allocated object.
 *
 * @param addr
 * @return struct memory_object_header*
 */
static struct memory_object_header *get_header(uint8_t *addr) {
    struct memory_object_header *moh = (struct memory_object_header *) (addr - sizeof(struct memory_object_header));

    return moh;
}

/**
 * @brief Free a previously-allocated memory object.
 *
 * @param addr
 */
void object_free(uint8_t *addr) {
    struct memory_object_cache *cache;
    struct memory_object_header *moh;
    struct memory_object *mo;

    if (!addr)
        return;

    moh = get_header(addr);
    cache = &memory_object_caches[moh->order - MIN_MEMORY_OBJECT_ORDER];

    mo = object_remove_used(cache, addr);
    if (!mo)
        return;

    object_prepend_free(cache, mo);
}

void init_memory_object(struct memory_object *object, const int order, const int size, struct memory_object *next_obj) {
    object->header.order = order;
    object->header.size = size;
    object->header.next = next_obj;
}

void memory_object_cache_init(struct memory_object_cache *cache, int order) {
    int init_object_count = 0, consumed = 0, skip_size;
    uint8_t *header_addr, *next_header_addr;
    struct mem_block *object_block;
    int object_block_size;

    object_block = __zone_alloc(&order_zones[_highest_initialized_zone_order]);
    if (!object_block) {
        print_string("Cache init failed on __zone_alloc for ");
        print_int32(order);
        print_string("\n");
        return;
    }
    object_block_size = ORDER_SIZE(object_block->order);
    skip_size = sizeof(struct memory_object_header) + (1 << order);

    if (skip_size > object_block_size)
        return;

    header_addr = NULL;
    next_header_addr = (uint8_t *) object_block->addr;

    cache->free_objects = (struct memory_object *) next_header_addr;
    cache->object_block_ptr = object_block;
    cache->object_size = 1 << order;
    cache->used_objects = NULL;
    cache->order = order;
    cache->free = 0;
    cache->used = 0;

    do {
        header_addr = next_header_addr;
        next_header_addr = header_addr + skip_size;

        init_memory_object((struct memory_object*)header_addr, cache->order, cache->object_size, (struct memory_object *) next_header_addr);
        cache->free++;

        init_object_count++;
        consumed += skip_size;
    } while (consumed + skip_size < object_block_size);

    ((struct memory_object *)header_addr)->header.next = NULL;
}

static void setup_memory_object_caches(void) {
    int order;

    for (int i = 0; i <= MEMORY_OBJECT_ORDER_RANGE; i++) {
        order = i + MIN_MEMORY_OBJECT_ORDER;

        memory_object_cache_init(&memory_object_caches[i], order);

        asm volatile ("nop");
    }
}

bool is_writeable(uint64_t addr) {
    char *ptr = (char *) to_addr_width(addr);
    char old = *ptr;
    char new = ~old;

    *ptr = new;

    return *ptr == new;
}

uint64_t find_max_writeable_address(struct bios_mem_map_entry *bmm_entry) {
    uint64_t addr_l = bmm_entry->base, addr_r = bmm_entry->base + bmm_entry->length - 1ull;
    uint64_t addr_m = (addr_l + addr_r) / 2;

    // Weird things happen when we try to find the max address we can read/write.
    // So, let's manually set it based on experience.
    // For example, when RAM is 4GiB, the largest bios memory map region is
    // [0x100000000, 0x140000000) but writes only seem to be successful up to
    // 0x10000b000. So we return 0x10000b000 - 1 here.
    if (bmm_entry->length == 0x40000000ull)
        return 0x10000b000ull - 1;

    // We've seen that for RAM sizes greater than 4GiB, i.e. >= 5GiB
    // the last (largest) region in the memory map extends to the RAM size + 1GiB
    // but the cpu is not happy to read/write that extra 1GiB.
    if (bmm_entry->length > 0x40000000)
        addr_r -= 0x40000000;

    if (is_writeable(addr_l) && is_writeable(addr_r)) {
        // We think addr_r is the max _writeable_addr but let's make sure.
        for (uint64_t a = addr_l; a <= addr_r; a++)
            SPIN_ON (!is_writeable(a));
        
        return addr_r;
    }

    do {
        if (!(is_writeable(addr_l) && !is_writeable(addr_r))) {
            print_string("Invalid range.\n");
            break;
        }

        addr_m = (addr_l + addr_r) / 2;
        if (is_writeable(addr_m))
            addr_l = addr_m;
        else
            addr_r = addr_m;

    } while (addr_r - addr_l >= 2);

    if (is_writeable(addr_l) && !is_writeable(addr_r)) {
        print_string("success!\n");
        print_ptr((void *) to_addr_width(addr_l)); print_string(" is writeable.\n");
        print_ptr((void *) to_addr_width(addr_r)); print_string(" is not writeable.\n");
        return addr_l;
    } else {
        print_string("failure:");
        print_ptr((void *) to_addr_width(addr_l)); print_string("<-l\n");
        print_ptr((void *) to_addr_width(addr_r)); print_string("<-r\n");
        return bmm_entry->base + 0xb000;
    }
}

/**
 * @brief Configure segmentation and paging related business and
 * print a bunch of stuff that might be useful to see (although we can probably
 * get all the same information via gdb so maybe not so useful).
 * 
 * Perhaps a couple of TODO:(s) here are:
 * 	i) Check for and return on errors.
 */
void init_mm(void) {
    print_string("mem_map_buf_entry_count="); 	print_int32(mem_map_buf_entry_count);
    print_string("\n");
    print_string("mem_map_buf_addr=");		    print_int32(mem_map_buf_addr);
    print_string("\n");

    bmm = (struct bios_mem_map_entry *)(pa_t)mem_map_buf_addr;
    for (int i = 0; i < mem_map_buf_entry_count; i++) {
        print_string("entry "); 		print_int32(i); 			print_string(" has base "); print_uint(bmm[i].base);
        print_string(" and length "); 	print_uint(bmm[i].length);
        print_string(" (avail="); 		print_int32(bmm[i].type); 	print_string(")\n");

        if (bmm[i].type == 1)
            _available_memory += bmm[i].length;

        if (i == mem_map_buf_entry_count - 1)
           _max_available_phy_addr = find_max_writeable_address(&bmm[i]);
    }

    init_page_usage_bitmap();

    print_string("_bss_start=");	print_int32(addr_to_u32(&_bss_start));	print_string(",");
    print_string("_bss_end="); 		print_int32(addr_to_u32(&_bss_end));		print_string(",");
    print_string("_text_start="); 	print_int32(addr_to_u32(&_text_start));	print_string(",");
    print_string("_text_end="); 	print_int32(addr_to_u32(&_text_end));		print_string(",");
    print_string("_data_start="); 	print_int32(addr_to_u32(&_data_start));	print_string(",");
    print_string("_data_end=");		print_int32(addr_to_u32(&_data_end));		print_string("\n");

    _bss_length = addr_to_u64(&_bss_end) - addr_to_u64(&_bss_start);
    _text_length = addr_to_u64(&_text_end) - addr_to_u64(&_text_start);
    _data_length = addr_to_u64(&_data_end) - addr_to_u64(&_data_start);

#ifdef CONFIG32
    setup_and_load_pm_gdt();
    setup_and_enable_paging();
#endif

    /* Set up structures for dynamic memory allocation and de-allocation. */
    setup_zone_alloc_free();

    uint64_t kernel_static_memory = _bss_length + _text_length + _data_length + _interrupt_stacks_length + (_interrupt_stacks_begin - addr_to_u64(&_bss_end));
                                                                                                  /* Nothing fits into this region. Plus, this is pretty */
                                                                                                  /* low memory and we would not allocate from here.     */
    uint64_t wasted_memory = _available_memory - _zone_designated_memory - kernel_static_memory - bmm[0].length;

    print_string(" Available memory="); 
    print_int64(_available_memory);
    print_string("\n");
    print_string("Designated memory=");
    print_int64(_zone_designated_memory);
    print_string("\n");
    print_string("    Wasted memory=");
    print_int64(wasted_memory);
    print_string("\n");

    /* Set up structures for dynamic allocation of small memory sizes. */
    setup_memory_object_caches();
}
