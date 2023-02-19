#include "mm.h"
#include <drivers/disk/disk.h>
#include <fs/filesystem.h>
#include "print.h"
#include "string.h"

extern struct order_zone order_zones[];

extern struct fs_master_record master_record;

static struct order_zone *get_auxillary_zone(struct order_zone *test_zone) {
    int auxillary_storage_needed = test_zone->num_blocks * sizeof(struct mem_block *);
    int auxillary_zone_order = 0;

    // Find a zone where 1 block is all we need to store test data. Using 1
    // block minimizes the risk of colliding with a latent mm bug.
    while ((1 << (auxillary_zone_order + PAGE_SIZE_SHIFT)) < auxillary_storage_needed)
        auxillary_zone_order++;

    if (auxillary_zone_order >= MAX_ORDER) {
        print_string("Skipping test, no zone big enough.");
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

        failed = failed || (addr[i] != new);
    }

    return failed;
}

static bool mem_zone_test(void) {
    /**
     * Memory allocation tests!
     * 
     * The 'auxillary' zone is memory we allocate to store data to facilitate
     * the test. Obviously, since we are testing the mm's reliability, this is
     * risky as it relies on the mm, but what choice do we have?
    */
    struct mem_block *auxillary_block, **auxillary_storage_region;
    struct order_zone *test_zone, *auxillary_zone;
    int test_zone_order;
    bool failed = false;
    int to_alloc;

    /**
     * Test 1:
     * 
     * Simple alloc and free, no auxillary zone necessary.
     * The empty comments are to make it easy to comment out the test.
     */
    /**/
    {
        test_zone_order = MAX_ORDER;
        test_zone = &order_zones[test_zone_order];

        struct mem_block *block = zone_alloc(ORDER_SIZE(test_zone_order));

        test_read_write(block);

        zone_free(block);
    }
    /**/

    /**
     * Test 2:
     * 
     * Exhaust the highest order zone, zone 7, and fail on the next alloc from this
     * zone.
     * The empty comments are to make it easy to comment out the test.
     */
    /**/
    {
        test_zone_order = MAX_ORDER;
        test_zone = &order_zones[test_zone_order];

        auxillary_zone = get_auxillary_zone(test_zone);
        if (auxillary_zone == test_zone)
            goto skip_test2;
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
    }
    /**/
    /* END OF TEST 2. */
skip_test2:

    /**
     * Test 3:
     * 
     * Exhaust second-highest-order zone, zone 6, request one more block which
     * should trigger a zone_brrow from zone 7. A free on the split block should
     * trigger a zone_collect.
     */
    /**/
    {
        test_zone_order = MAX_ORDER - 1;
        test_zone = &order_zones[test_zone_order];

        auxillary_zone = get_auxillary_zone(test_zone);
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
    }
    /* END OF TEST 3. */

    return failed;
}

static void mem_test(void) {
    bool zone_result, object_result;

    zone_result = mem_zone_test();
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
void disk_test(void) {

    print_string("sector_bitmap size="); print_int32(master_record.sector_bitmap_size);
    print_string("\n");

    for (int i = 0; i <= 5; i++) {
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

        zone_free(block);
    }
}

void system_test(void) {
    mem_test();

    disk_test();
}
