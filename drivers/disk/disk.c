#include "disk.h"

#include <kernel/error.h>
#include <kernel/irq.h>
#include <kernel/low_level.h>
#include <kernel/print.h>
#include <kernel/string.h>

#define FLUSH_BUFFER_SIZE SECTOR_SIZE

static long long unsigned int interrupt_count = 0;
static char flush_buffer[FLUSH_BUFFER_SIZE];

#define SHOW_REGISTER8(config, name) {								   \
	uint8_t name ## _val = port_byte_in(config->name ## _port);					   \
	print_string(#name); print_string(": "); print_int32(name ## _val); print_string("\n");\
}

#define SHOW_REGISTER16(config, name) {								   \
	uint16_t name ## _val = port_word_in(config->name ## _port);					   \
	print_string(#name); print_string(": "); print_int32(name ## _val); print_string("\n");\
}

static void __display_registers(struct ata_port_config *config) {
	SHOW_REGISTER8(config, sector_count);
	SHOW_REGISTER8(config, drive_select);
	SHOW_REGISTER8(config, lba_high);
	SHOW_REGISTER8(config, lba_low);
	SHOW_REGISTER8(config, lba_mid);
	SHOW_REGISTER8(config, command);
	SHOW_REGISTER8(config, status);
	SHOW_REGISTER8(config, error);
	SHOW_REGISTER16(config, data);
}

static void __poll_status_register(struct ata_port_config *config) {
	uint8_t hd_ctrl_status;

	while(((hd_ctrl_status = port_byte_in(config->status_port)) & 0xC0) != 0x40) {
		// print_string("HD_ctrl not ready. Status:"); print_int32(hd_ctrl_status); print_string("\n");
	}
}

static void assign_ata_ports(enum disk_channel channel, struct ata_port_config *config) {
	switch (channel) {
	case PRIMARY: 
		config->sector_count_port = HD_PORT_SECT_COUNT_PRIMARY;
		config->drive_select_port = HD_PORT_DRV_HEAD_PRIMARY;
		config->lba_high_port = HD_PORT_LBA_HIGH_PRIMARY;
		config->lba_low_port = HD_PORT_LBA_LOW_PRIMARY;
		config->lba_mid_port = HD_PORT_LBA_MID_PRIMARY;
		config->command_port = HD_PORT_COMMAND_PRIMARY;
		config->status_port = HD_PORT_STATUS_PRIMARY;
		config->error_port = HD_PORT_ERROR_PRIMARY;
		config->data_port = HD_PORT_DATA_PRIMARY;
		break;
	case SECONDARY:
		config->sector_count_port = HD_PORT_SECT_COUNT_SECONDARY;
		config->drive_select_port = HD_PORT_DRV_HEAD_SECONDARY;
		config->lba_high_port = HD_PORT_LBA_HIGH_SECONDARY;
		config->lba_low_port = HD_PORT_LBA_LOW_SECONDARY;
		config->lba_mid_port = HD_PORT_LBA_MID_SECONDARY;
		config->command_port = HD_PORT_COMMAND_SECONDARY;
		config->status_port = HD_PORT_STATUS_SECONDARY;
		config->error_port = HD_PORT_ERROR_SECONDARY;
		config->data_port = HD_PORT_DATA_SECONDARY;
		break;
	default:
		print_string("Bad drive selection. Neither primary or secondary chosen.\n");
	}
}

static enum sys_error __read_from_disk(enum disk_channel channel, enum drive_class class, lba_t block_address, int n_bytes, void* buffer) {
	struct ata_port_config config;
	uint8_t n_sectors;
	uint8_t command;
	int flush;

	if (n_bytes <= 0)
		return -1;

	disable_interrupts();
	assign_ata_ports(channel, &config);

	n_sectors = (n_bytes >> 9) + (n_bytes & 0x1ff ? 1 : 0);

	// According to https://wiki.osdev.org/ATA_PIO_Mode#Hardware, we need to
	// select the correct driver first before reading from the status register.
	// Then, because the device might need some time to respond to the drive
	// select, we need to read the status register at least 15 times before
	// beginning to trust its output.
	port_byte_out(config.drive_select_port, (class == SLAVE ? 0xF0 : 0xE0) | ((block_address >> 24) & 0x0f));
	for (int i = 0; i < 14; i++)
		port_byte_in(config.status_port);

	__poll_status_register(&config);

	port_byte_out(config.error_port, 0x0);
	port_byte_out(config.sector_count_port, n_sectors);
	port_byte_out(config.lba_low_port, block_address);
	port_byte_out(config.lba_mid_port, block_address >> 8);
	port_byte_out(config.lba_high_port, block_address >> 16);

	// Do we need this?
	__poll_status_register(&config);
	port_byte_out(config.sector_count_port, n_sectors);

	// Send read command to controller.
	command = n_sectors > 1 ? HD_READ_MULTIPLE : HD_READ;
	port_byte_out(config.command_port, command);

	// Do we need this? According to https://wiki.osdev.org/ATA_PIO_Mode#Hardware, we do.
	// "When a driver issues a PIO read or write command, it needs to wait
	//  until the drive is ready before transferring data."
	__poll_status_register(&config);

	flush = SECTOR_SIZE - (n_bytes % SECTOR_SIZE) /* SECTOR_SIZE */;

	insw(config.data_port, buffer, (n_bytes >> WORD_TO_BYTE_SHIFT) + (n_bytes & 0x1 ? 1 : 0));
	// From experience, we have to read the entirety of all the sectors we indicate
	// to the sector count port. Until we do this, writes to registers won't work
	// and the device will try to fulfill subsequent read commands starting from
	// where a previous "incomplete" read left off.
	if (flush)
		insw(config.data_port, flush_buffer, flush >> WORD_TO_BYTE_SHIFT);

	enable_interrupts();
	return NONE;
}

/**
 * @brief Write n_sectors sectors to disk.
 * The finest granularity of disk writes supported by this driver
 * is at the sector level.
 *
 * There are 2 main reasons for this:
 *
 * 1. Writes of less than a sector leave the device in a state which
 * can sometimes cause issues with both subsequent reads and writes.
 *
 * 2. There appears to be an issue writing bytes (outsb) to the data
 * port/register - it simply doesn't work. So instead write words
 * (outsb).
 *
 * It might be worth experimenting with outsl which would be more
 * efficient theoretically, but I don't want to spend the time finding
 * out that there is some outsl quirk for this specific port/register in
 * this specific device like there appears to be with outsb.
 *
 * @param channel
 * @param block_address
 * @param n_sectors
 * @param buffer
 * @return enum sys_error
 */
static enum sys_error __write_to_disk(enum disk_channel channel, enum drive_class class, lba_t block_address, int n_sectors, void *buffer) {
	struct ata_port_config config;
	int command, flush;

	if (n_sectors < 0)
		return -1;

	disable_interrupts();
	assign_ata_ports(channel, &config);

	port_byte_out(config.drive_select_port, (class == SLAVE ? 0xF0 : 0xE0) | ((block_address >> 24) & 0x0F));
	for (int i = 0; i < 14; i++)
		port_byte_in(config.status_port);

	__poll_status_register(&config);

	port_byte_out(config.error_port, 0x0);
	port_byte_out(config.sector_count_port, n_sectors);
	port_byte_out(config.lba_low_port, block_address);
	port_byte_out(config.lba_mid_port, block_address >> 8);
	port_byte_out(config.lba_high_port, block_address >> 16);

	__poll_status_register(&config);

	// Send write command to controller.
	command = n_sectors > 1 ? HD_WRITE_MULTIPLE : HD_WRITE;
	port_byte_out(config.command_port, command);

	__poll_status_register(&config);

	outsw(config.data_port, buffer, (n_sectors * SECTOR_SIZE) >> WORD_TO_BYTE_SHIFT);

	enable_interrupts();
	return NONE;
}

/**
 * write_to_storage_disk - Write to ATA/IDE hard disk from buffer.
 * 
 * "Storage disk" (as opposed to boot disk) is arbitrarily designated
 * as the slave drive on the controller bus.
 *
 * @block_address:
 * @n_bytes:
 * @buffer:
 */
int write_to_storage_disk(lba_t block_address, int n_bytes, void* buffer) {
	int full_sectors = n_bytes / SECTOR_SIZE;
	int rem = n_bytes % SECTOR_SIZE;
	int error = 0;

	if (full_sectors)
		error = __write_to_disk(PRIMARY, SLAVE, block_address, full_sectors, buffer);

	if (rem) {
		int final_write_block_idx = block_address + full_sectors;

		clear_buffer(flush_buffer, FLUSH_BUFFER_SIZE);

		read_from_storage_disk(final_write_block_idx, SECTOR_SIZE, flush_buffer);

		memory_copy((char *)buffer + full_sectors * SECTOR_SIZE, flush_buffer, rem);

		error = __write_to_disk(PRIMARY, SLAVE, final_write_block_idx, 1, flush_buffer) | error;
	}

	return error;
}

/**
 * read_from_storage_disk - Read from ATA/IDE hard disk into buffer.
 *
 * "Storage disk" (as opposed to boot disk) is arbitrarily designated
 * as the slave drive on the controller bus.
 * 
 * @block_address:
 * @n_bytes:
 * @buffer:
 */
int read_from_storage_disk(lba_t block_address, int n_bytes, void* buffer) {
	return __read_from_disk(PRIMARY, SLAVE, block_address, n_bytes, buffer);
}

static inline void disk_irq_handler(struct registers* r) {
	interrupt_count++;
}

static void install_disk_irq_handler(void) {
	install_irq(14, disk_irq_handler);
}

/**
 * init_disk
 *
 * Perhaps a TODO: implement more useful disk setup.
 */
void init_disk(void) {
	install_disk_irq_handler();
}
