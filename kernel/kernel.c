// A simple kernel.
#include "system.h"

#include "interrupts.h"
#include "mm.h"
#include "print.h"
#include "string.h"
#include "task.h"
#include "timer.h"

#include <drivers/disk/disk.h>
#include <drivers/keyboard/keyboard.h>
#include <fs/fnode.h>

#include "shell/shell.h"

va_t APP_START_VIRT_ADDR = 0x30000000;
pa_t APP_START_PHY_ADDR = 0x20000000;
va_range_sz_t APP_STACK_SIZE = 8192;
va_range_sz_t APP_HEAP_SIZE = 16384;

static char *kernel_init_message = "Kernel initialized successfully.\n";
static char *kernel_load_message = "Kernel loaded and running.\n";

extern struct folder root_folder;

/**
 * init - Initialize system components.
 */
void init(void) {
	/* Set up fault handlers and interrupt handlers */
	/* but do not enable interrupts. */
	init_interrupts();

	/* Set up system's timer. */
	init_timer();

	/* Set up memory management. */
	init_mm();

	/* Set up disk. */
	init_disk();

	/* Set up task management. */
	init_task_system();

	/* Set up fs. */
	init_fs();
	
	/* Setup keyboard */
	init_keyboard();

	/* Let the fun begin, enable interrupts. */
	start_interrupts();
}

extern struct order_zone order_zones[];

/**
 * main - main kernel execution starting point.
 */
int main(void) {
	print_string(kernel_load_message);
	init();
	print_string(kernel_init_message);

	/*
	struct file file;
	int status;

	status = find_file(&root_folder, "app.bin", &file);
	if (status != 0) {
		print_string("file not found.");
		PAUSE();
	}

	execute_binary_file(&file);
	*/
	mem_test();
	exec_main_shell();
}

void mem_test(void) {
	/* Memory allocation tests!
	
		The 'auxillary' zone is memory we allocate to store data to facilitate
		the test. Obviously, since we are testing the mm's reliability, this is
		risky as it relies on the mm, but what choice do we have?
	*/
	struct mem_block *auxillary_block, **auxillary_storage_region;
	struct order_zone *test_zone, *auxillary_zone;
	int test_zone_order, auxillary_zone_order;
	int auxillary_storage_needed;
	int to_alloc;

	/**
	 * Test 1:
	 * 
	 * Simple alloc and free, no auxillary zone necessary.
	 * The empty comments are to make it easy to comment out the test.
	 */
	/**/
	{
		test_zone_order = 7;
		test_zone = &order_zones[test_zone_order];

		struct mem_block *block = zone_alloc(test_zone_order);
		zone_free(block);
	}
	/**/

	/**
	 * Test 2:
	 * 
	 * Exhaust the highest order zone, zone 7, and fail on the next alloc from this
	 * zone.
	 * The empty comments are tomake it easy to comment out the test.
	 */
	/**/
	{
		test_zone_order = 7;
		test_zone = &order_zones[test_zone_order];
		auxillary_storage_needed = test_zone->num_blocks * sizeof(struct mem_block *);

		// Find a zone where 1 block is all we need to store test data. Using 1
		// block minimizes the risk of colliding with a latent mm bug.
		auxillary_zone_order = 0;
		while ((1 << (auxillary_zone_order + PAGE_SIZE_SHIFT)) < auxillary_storage_needed)
			auxillary_zone_order++;

		if (auxillary_zone_order >= MAX_ORDER) {
			print_string("Skipping test, no zone big enough.");
			goto skip_test2;
		}
		print_string("auxillary_zone_order=");
		print_int32(auxillary_zone_order);
		print_string("\n");

		auxillary_zone = &order_zones[auxillary_zone_order];
		auxillary_block = zone_alloc(auxillary_zone_order);
		auxillary_storage_region = auxillary_block->addr;

		to_alloc = test_zone->num_blocks;
		// Exhaust the zone - allocate all its blocks.
		print_string("alloced:");
		for (int i = 0; i < to_alloc; i++) {
			struct mem_block *tmp_block = zone_alloc(test_zone_order);

			// Let's not print everthing; zone's have thousands of blocks.
			if (i > to_alloc - 8) {
				print_int32(tmp_block);
				print_string((i < to_alloc - 1 ? "," : ""));
			}

			// Store the block away, we'll need it when we want to free all the memory.
			auxillary_storage_region[i] = tmp_block;
		}
		print_string("\n");

		// We have allocated all the blocks in the highest order zone. It has no zone
		// it can borrow from, so the allocation should fail.
		struct mem_block *failed_alloc_block = zone_alloc(test_zone_order);
		if (!failed_alloc_block)
			print_string("allocation failed, [success]\n");
		else
			print_string("allocation passed, [failure]\n");

		// Free all the blocks allocated from test_zone.
		print_string("freed:");
		for (int i = 0; i < to_alloc; i++) {
			struct mem_block *tmp_block = auxillary_storage_region[i];

			if (i > to_alloc - 8) {
				print_int32(tmp_block);
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
		test_zone_order = 6;
		test_zone = &order_zones[test_zone_order];
		auxillary_storage_needed = test_zone->num_blocks * sizeof(struct mem_block *);

		auxillary_zone_order = 0;
		while ((1 << (auxillary_zone_order + PAGE_SIZE_SHIFT)) < auxillary_storage_needed)
			auxillary_zone_order++;

		if (auxillary_zone_order >= MAX_ORDER) {
			print_string("Skipping test, no zone big enough.");
			goto skip_test3;
		}
		print_string("auxillary_zone_order=");
		print_int32(auxillary_zone_order);
		print_string("\n");

		auxillary_zone = &order_zones[auxillary_zone_order];
		auxillary_block = zone_alloc(auxillary_zone_order);
		auxillary_storage_region = auxillary_block->addr;

		to_alloc = test_zone->num_blocks;
		print_string("alloced: ");
		for (int i = 0; i < to_alloc; i++) {
			struct mem_block *tmp_block = zone_alloc(test_zone_order);

			// Let's not print everthing; zone's have thousands of blocks.
			if (i > to_alloc - 8) {
				print_int32(tmp_block);
				print_string((i < to_alloc - 1 ? "," : ""));
			}

			auxillary_storage_region[i] = tmp_block;
		}
		print_string("\n");

		struct mem_block *test_block = zone_alloc(test_zone_order);
		if (test_block)
			print_string("allocation passed, [success]\n");
		else
			print_string("allocation failed, [failure]\n");
		zone_free(test_block);

		print_string("freed: ");
		for (int i = 0; i < to_alloc; i++) {
			struct mem_block *tmp_block = auxillary_storage_region[i];

			// Let's not print everthing; zone's have thousands of blocks.
			if (i > to_alloc - 8) {
				print_int32(tmp_block);
				print_string((i < to_alloc - 1 ? "," : ""));
			}

			zone_free(tmp_block);
		}
		print_string("\n");
		zone_free(auxillary_block);
	}
	/* END OF TEST 3. */

skip_test3:
	return;
}
