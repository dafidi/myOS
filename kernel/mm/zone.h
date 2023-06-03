#ifndef __ZONE_H__
#define __ZONE_H__

#include <kernel/system.h>

// Zone level stuff
#define MAX_ORDER 8
#define DEFAULT_PAGES_PER_ZONE ((SYSTEM_NUM_PAGES) / (MAX_ORDER + 1))
#define ORDER_SIZE(s) (1 << (PAGE_SIZE_SHIFT + (s)))
#define OBJECT_ORDER_SIZE(s) (1 << (s))

enum mem_block_state {
	FREE,
	USED
};

struct mem_block {
	enum mem_block_state state;
	struct mem_block *next;
	uint8_t order;
	uint8_t trueorder; /* A block may be split in 2, decrementing its
						  order by 1. trueorder lets us know to merge this block
						  with its buddy when it is being freed. This
						  merging/coalescing may occur recursively. Never,
						  ever write/change trueorder, except when splitting
						  a block into 2 produces a "new" block object. Its
						  trueorder is inherited from the parent block.		*/
	pa_t addr;
};

struct order_zone {
	struct mem_block *free_list;	/* List of mem_blocks.					*/
	struct mem_block *used_list;	/* List of mem_blocks.					*/
	uint64_t phy_mem_start;
	uint8_t order;					/* Power-of-2 indicator or size of the 	*/
									/* blocks in this region. The blocks 	*/
									/* have size PAGE_SIZE * 2**order.		*/
	uint32_t num_blocks;
	uint32_t free;
	uint32_t used;
	uint32_t state;
};

enum order_zone_state {
	INITIALIZED = 0x1,
};

void setup_zone_alloc_free(void);

#endif // __ZONE_H__
