#include "mm/mm.h"
#include <drivers/disk/disk.h>
#include <fs/filesystem.h>
#include "print.h"
#include "string.h"

extern int _highest_initialized_zone_order;

extern struct order_zone order_zones[];

extern struct fs_master_record master_record;

static struct order_zone *get_auxillary_zone(struct order_zone *test_zone) {
    int auxillary_storage_needed = test_zone->num_blocks * sizeof(struct mem_block *);
    int auxillary_zone_order = 0;

    // Find a zone where 1 block is all we need to store test data. Using 1
    // block minimizes the risk of colliding with a latent mm bug.
    while ((1 << (auxillary_zone_order + PAGE_SIZE_SHIFT)) < auxillary_storage_needed)
        auxillary_zone_order++;

    if (auxillary_zone_order >= _highest_initialized_zone_order ||
        auxillary_zone_order >= test_zone->order) {
        print_string("Skipping test, no zone big enough.\n");
        return NULL;
    }

    print_string("auxillary_zone_order=");
    print_int32(auxillary_zone_order);
    print_string("\n");

    return &order_zones[auxillary_zone_order];
}

static bool alloc_free(int sz) {
    bool failed = false;
    uint8_t *mem;

    mem = object_alloc(sz);
    if (!mem)
        failed = true;

    object_free(mem);
    return failed;
}

static bool mem_object_test(void) {
    bool failed = false;

    for (int i = MIN_MEMORY_OBJECT_ORDER; i <= MAX_MEMORY_OBJECT_ORDER; i++) {
        bool res = alloc_free(OBJECT_ORDER_SIZE(i));

        failed |= res;

        if (res) {
            print_string("a/f failed, n="); print_int32(i); print_string("\n");
        }
    }

    return failed;
}

static bool test_read_write(struct mem_block *block) {
    const int num_bytes = ORDER_SIZE(block->order);
    char *addr = (char *) block->addr;
    bool failed = false;

    for (int i = 0; i < num_bytes; i++) {
        char old = addr[i];
        char new = ~old;
        addr[i] = new;

        if (addr[i] != new) {
            failed = true;
            print_string("Failed r/w at i,addr,old,new=");
            print_int32(i);
            print_string(",");
            print_ptr(&addr[i]);
            print_string(",");
            print_int64(old);
            print_string(",");
            print_int64(new);
            print_string("\n");
            while(1);
        }
    }

    return failed;
}

static int block_in_list(struct mem_block *block, struct mem_block *list) {
    struct mem_block *block_ptr = list;
    int count = 0;

    while (block_ptr) {
        if (block_ptr == block)
            count++;
        block_ptr = block_ptr->next;
    }

    return count;
}

static bool mem_zone_test_alloc_free(struct order_zone *zone) {
    int used_presence = 0, free_presence = 0;
    struct mem_block *alloced_block;
    int free_before, free_after;
    int used_before, used_after;
    bool failed = false;

    free_before = zone->free;
    used_before = zone->used;

    alloced_block = zone_alloc(ORDER_SIZE(zone->order));
    SPIN_ON(!alloced_block);

    free_after = zone->free;
    used_after = zone->used;

    SPIN_ON(free_before != free_after + 1);
    SPIN_ON(used_before != used_after - 1)

    // The allocated block must not be among the free blocks.
    free_presence = block_in_list(alloced_block, zone->free_list);
    SPIN_ON(free_presence);

    // The allocated blocks must occur among the used blocks exactly once.
    used_presence = block_in_list(alloced_block, zone->used_list);
    SPIN_ON(used_presence != 1);

    SPIN_ON(test_read_write(alloced_block));
    free_before = zone->free;
    used_before = zone->used;

    zone_free(alloced_block);

    free_after = zone->free;
    used_after = zone->used;

    SPIN_ON(free_before != free_after - 1);
    SPIN_ON(used_before != used_after + 1)

    return failed;
}

static bool mem_zone_test_exhaust_highest_order_zone(void) {
    struct mem_block *auxillary_block, **auxillary_storage_region;
    struct order_zone *test_zone, *auxillary_zone;
    int test_zone_order, to_alloc;
    bool failed = false;

    test_zone_order =  _highest_initialized_zone_order;
    test_zone = &order_zones[test_zone_order];
    auxillary_zone = get_auxillary_zone(test_zone);
    if (!auxillary_zone)
        goto skip_test;

    auxillary_block = zone_alloc(ORDER_SIZE(auxillary_zone->order));
    auxillary_storage_region = (struct mem_block **) auxillary_block->addr;
    to_alloc = test_zone->free;

    // Exhaust the zone - allocate all its blocks.
    print_string("alloced:");
    for (int i = 0; i < to_alloc; i++) {
        struct mem_block *tmp_block = zone_alloc(ORDER_SIZE(test_zone_order));
        // Let's not print everthing; zone's have thousands of blocks.
        if (i > to_alloc - 8) {
            print_int32((pa_t) tmp_block);
            print_string((i < to_alloc - 1 ? "," : ""));
        }
        // Store the block away, we'll need it when we want to free all the memory.
        auxillary_storage_region[i] = tmp_block;
    }
    print_string("\n");

    // We have allocated all the blocks in the highest order zone. It has no zone
    // it can borrow from, so the allocation should fail.
    struct mem_block *failed_alloc_block = zone_alloc(ORDER_SIZE(test_zone_order));
    if (!failed_alloc_block) {
        print_string("allocation failed [success]\n");
    } else {
        print_string("allocation passed [failure]\n");
        failed = true;
    }

    // Free all the blocks allocated from test_zone.
    print_string("freed:");
    for (int i = 0; i < to_alloc; i++) {
        struct mem_block *tmp_block = auxillary_storage_region[i];
        if (i > to_alloc - 8) {
            print_int32((pa_t) tmp_block);
            print_string((i < to_alloc - 1 ? "," : ""));
        }
        zone_free(tmp_block);
    }
    print_string("\n");

    zone_free(auxillary_block);

skip_test:
    return failed;
}

static bool mem_zone_test_exhaust_second_highest_order_zone(void) {
    struct mem_block *auxillary_block, **auxillary_storage_region;
    struct order_zone *test_zone, *auxillary_zone;
    int test_zone_order;
    bool failed = false;
    int to_alloc;

    test_zone_order = _highest_initialized_zone_order - 1;
    test_zone = &order_zones[test_zone_order];

    auxillary_zone = get_auxillary_zone(test_zone);
    if (!auxillary_zone)
        goto skip_test;

    auxillary_block = zone_alloc(ORDER_SIZE(auxillary_zone->order));
    auxillary_storage_region = (struct mem_block **) auxillary_block->addr;
    to_alloc = test_zone->free;

    print_string("alloced: ");
    for (int i = 0; i < to_alloc; i++) {
        struct mem_block *tmp_block = zone_alloc(ORDER_SIZE(test_zone_order));
        // Let's not print everthing; zone's have thousands of blocks.
        if (i > to_alloc - 8) {
            print_int32((pa_t) tmp_block);
            print_string((i < to_alloc - 1 ? "," : ""));
        }
        auxillary_storage_region[i] = tmp_block;
    }
    print_string("\n");

    struct mem_block *test_block = zone_alloc(ORDER_SIZE(test_zone_order));
    if (test_block) {
        print_string("allocation passed [success]\n");
    } else {
        print_string("allocation failed [failure]\n");
        failed = true;
    }
    zone_free(test_block);

    print_string("freed: ");
    for (int i = 0; i < to_alloc; i++) {
        struct mem_block *tmp_block = auxillary_storage_region[i];
        // Let's not print everthing; zone's have thousands of blocks.
        if (i > to_alloc - 8) {
            print_int32((pa_t) tmp_block);
            print_string((i < to_alloc - 1 ? "," : ""));
        }
        zone_free(tmp_block);
    }
    print_string("\n");
    zone_free(auxillary_block);

skip_test:
    return failed;
}

static bool mem_zone_test_suite(void) {
    /**
     * Memory allocation tests!
     * 
     * The 'auxillary' zone is memory we allocate to store data to facilitate
     * the test. Obviously, since we are testing the mm's reliability, this is
     * risky as it relies on the mm, but what choice do we have?
    */
    struct order_zone *test_zone;
    int test_zone_order;
    bool failed = false;

    /**
     * Simple alloc and free, no auxillary zone necessary.
     * The empty comments are to make it easy to comment out the test.
     */
    /**/
    {
        test_zone_order = _highest_initialized_zone_order;
        test_zone = &order_zones[test_zone_order];
        if (mem_zone_test_alloc_free(test_zone)) {
            print_string("zone_alloc_free failed\n");
            failed = true;
        }

    }
    /**/

    /**
     * Exhaust the highest order zone, zone 7, and fail on the next alloc from this
     * zone.
     * The empty comments are to make it easy to comment out the test.
     */
    /**/
    failed = failed || mem_zone_test_exhaust_highest_order_zone();

    /**
     * Exhaust second-highest-order zone, zone 6, request one more block which
     * should trigger a zone_brrow from zone 7. A free on the split block should
     * trigger a zone_collect.
     */
    /**/
    failed = failed || mem_zone_test_exhaust_second_highest_order_zone();

    return failed;
}

static void mem_test(void) {
    bool zone_result, object_result;

    zone_result = mem_zone_test_suite();
    object_result = mem_object_test();
    
    print_string("Zone test: "); print_string(zone_result ? "failed" : "passed"); print_string(".\n");
    print_string("Object test: "); print_string(object_result ? "failed" : "passed"); print_string(".\n");
}

/**
 * @brief This test verifies reads from disks.
 * 
 * For each zone/order, read in bytes [0, ORDER_SIZE(order)) from the bitmap.
 * The bitmap is initialized with 8404996 sectors used - so 8404996 bit are set,
 * corresponding to 1050624 bytes which are all 0xff.
 * The largest zone allocates 524288 bytes so we can expect it to contain al 0xff
 * if the read works properly.
 * If we implement the ability to allocate larger blocks, we'll need to update
 * this test.
 * 
 */

extern bp();
void disk_test(void) {

    print_string("sector_bitmap size="); print_int32(master_record.sector_bitmap_size);
    print_string("\n");

    for (int i = 0; i <= _highest_initialized_zone_order; i++) {
        int first_err_pos = -1, last_err_pos = -1;
        int block_size, err_count = 0;
        struct mem_block *block;
        uint8_t *buffer;

        block_size = ORDER_SIZE(i);
        block = zone_alloc(block_size);
        buffer = (uint8_t *) block->addr;

        read_from_storage_disk(master_record.sector_bitmap_start_sector, block_size, buffer);

        for (int j = 0; j < block_size; j++) {
            if (buffer[j] != 0xff) {
                if (err_count == 0 )
                    first_err_pos = j;
                err_count++;
                last_err_pos = j;
            }
        }

        print_string("Zone ");  print_int32(i);
        print_string(" size="); print_int32(block_size);
        print_string(" first: "); print_int32(first_err_pos);
        print_string(" last: "); print_int32(last_err_pos);
        print_string(" errors="); print_int32(err_count);
        print_string("\n");

        bp();
        zone_free(block);
    }
}

void system_test(void) {
    mem_test();

    disk_test();

    bp();
}
