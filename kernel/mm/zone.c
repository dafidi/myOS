#include "zone.h"

#include <kernel/print.h>
#include <kernel/system.h>

extern struct bios_mem_map_entry *bmm;

extern pa_t mem_map_buf_addr;
extern unsigned int mem_map_buf_entry_count;

extern pa_t _interrupt_stacks_end;

extern uint64_t _available_memory;
extern uint64_t _max_available_phy_addr;

/**
 * An array of lists of mem_blocks. Each list of mem_blocks contains
 * mem_blocks of the same order. I.e. order_zones[i] is a list
 * of mem_blocks of size 2^i.
 * 
 * "MAX_ORDER + 1" because we want to be able to say "order_zones[MAX_ORDER]".
 */
struct order_zone order_zones[MAX_ORDER + 1];
struct mem_block mem_block_pool[SYSTEM_NUM_PAGES];

uint64_t _zone_designated_memory = 0;
int _highest_initialized_zone_order = 0;

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

void hlt() {
    while(1);
}

extern bp();

/**
 * @brief Add block to the end of zone's free_list.
 * 
 * @param zone 
 * @param block 
 */
static void zone_append_free(struct order_zone *zone, struct mem_block *block) {
    struct mem_block *free_tail = zone->free_list;

    int i = 0;
    if (free_tail) {
        while (free_tail->next) {
            if (free_tail->next > 0x100000000ull) {
                print_int32(i); print_string("[ERROR!]\n");
                bp();
            }
            free_tail = free_tail->next;
            i += 1;
        }
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
        // print_int32(block_index);
        // print_string("\n");
        // if (block_index == 12641) {
        //     print_string("breaking\n");
        //     while(1);
        // }
        zone->free++;
    }

    zone->free_list[mem_block_pool_index - num_block_pages].next = NULL;
    mark_zone_initialized(zone);

    /* It's nice to see the initialized zone's description. */
    print_order_zone(zone);
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
void setup_zone_alloc_free(void) {
    unsigned long long dynamic_memory_start =
        PAGE_ALIGN_UP((pa_t)_interrupt_stacks_end);

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
            print_string("addr="); print_ptr((void *)u64_to_addr(start_addr)); print_string(",");
            print_string("size="); print_ptr((void *)u64_to_addr(size)); print_string("\n");
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
