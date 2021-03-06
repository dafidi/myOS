#include "disk.h"

#include <drivers/screen/screen.h>
#include <kernel/error.h>
#include <kernel/string.h>
#include <kernel/irq.h>
#include <kernel/low_level.h>

// Helpful variables.
static char tmp[10] = "0000000";

static unsigned char hd_ctrl_status;

// #define LOGGING_ENABLED
#if defined(LOGGING_ENABLED)
static unsigned char hd_ctrl_error;
#define SHOW_DISK_CTRL_STATUS(msg) hd_ctrl_status = port_byte_in(status_port); \
																	 int_to_string(tmp, hd_ctrl_status, 9); \
																	 print(msg); print(tmp); print("\n");

#define SHOW_DISK_CTRL_ERROR(msg) hd_ctrl_error = port_byte_in(error_port); \
																	 int_to_string(tmp, hd_ctrl_error, 9); \
																	 print(msg); print(tmp); print("\n");
#else
#define SHOW_DISK_CTRL_STATUS(msg)
#define SHOW_DISK_CTRL_ERROR(msg)
#endif

static void assign_ports(enum disk_channel channel, uint16_t* drive_select_port,
	uint16_t* sector_count_port, uint16_t* lba_high_port, uint16_t* lba_low_port,
	uint16_t* lba_mid_port, uint16_t* command_port, uint16_t* status_port,
	uint16_t* error_port, uint16_t* data_port);

void init_disk(void) {
	install_disk_irq_handler();
	/* TODO: More intelligent disk setup.*/
}

void disk_irq_handler(struct registers* r) {
	print("[DISK IRQ]\n");
}

void install_disk_irq_handler(void) {
	install_irq(14, disk_irq_handler);
}

// Read from ATA/IDE hard disk into buffer.
void read_from_storage_disk(lba_t block_address, int n_bytes, void* buffer) {
	read_from_disk(PRIMARY, SLAVE, block_address, n_bytes, buffer);
}

// Write to ATA/IDE hard disk into buffer.
void write_to_storage_disk(lba_t block_address, int n_bytes, void* buffer) {
	write_to_disk(PRIMARY, SLAVE, block_address, n_bytes, buffer);
}

enum sys_error read_from_disk(enum disk_channel channel, enum drive_class class, lba_t block_address, int n_bytes, void* buffer) {
	uint16_t drive_select_port;
	uint16_t sector_count_port;
	uint16_t lba_high_port;
	uint16_t lba_low_port;
	uint16_t lba_mid_port;
	uint16_t command_port;
	uint16_t status_port;
	uint16_t error_port;
	uint16_t data_port;
	int read_command;
	int num_sectors;

	if (n_bytes <= 0) {
		return -1;
	}

	print("Reading from Disk...\n");
	disable_interrupts();
	assign_ports(channel,
							 &drive_select_port,
							 &sector_count_port,
							 &lba_high_port,
							 &lba_low_port,
							 &lba_mid_port,
							 &command_port,
							 &status_port,
							 &error_port,
							 &data_port);

	num_sectors = (n_bytes >> 9) + (n_bytes & 0x1ff ? 1 : 0);
	print("Reading "); print_int32(num_sectors); print(" sectors. \n");

	while((port_byte_in(status_port) & 0xc0) != 0x40) {
		int hd_ctrl_status = port_byte_in(status_port);
		int_to_string(tmp, hd_ctrl_status, 9);
		// print("hd_ctrl_status:"); print(tmp); print("\n");
		// print("Disk Controller busy...\n");
	}

	port_byte_out(error_port, 0x0);
	port_byte_out(sector_count_port, num_sectors);
	port_byte_out(lba_low_port, block_address);
	port_byte_out(lba_mid_port, block_address >> 8);
	port_byte_out(lba_high_port, block_address >> 16);
	port_byte_out(drive_select_port, (0xE0 | (class == SLAVE ? 0x10 : 0x0) | ((block_address >> 24) & 0x0f)));

	SHOW_DISK_CTRL_STATUS("STATUS [before read] STATUS:");
	SHOW_DISK_CTRL_ERROR("ERROR [before read] ERROR:");

	while ((hd_ctrl_status & 0xc0) != 0x40) { // Loop while controller is busy nor not ready.
		int_to_string(tmp, hd_ctrl_status, 9);
		// print("HD BUSY! STATUS:"); print(tmp); print("\n");
		hd_ctrl_status = port_byte_in(status_port);
	}

	// Send read command to controller.
	read_command = num_sectors > 1 ? HD_READ_MULTIPLE : HD_READ;
	port_byte_out(command_port, read_command);

	SHOW_DISK_CTRL_STATUS("STATUS [after read (1)] STATUS:");
	SHOW_DISK_CTRL_ERROR("ERROR [after read (1)] ERROR:");

	SHOW_DISK_CTRL_STATUS("STATUS [after read (2)] STATUS:");
	SHOW_DISK_CTRL_ERROR("ERROR [after read (2)] ERROR:");

	while ((hd_ctrl_status & 0xc0) != 0x40) { // Loop while controller is busy nor not ready.
		int_to_string(tmp, hd_ctrl_status, 9);
		print("HD BUSY! STATUS:"); print(tmp); print("\n");
		hd_ctrl_status = port_byte_in(status_port);
	}

	int_to_string(tmp, hd_ctrl_status, 9);
	print("HD READY! STATUS:"); print(tmp); print("\n");

	// print("before: buffer = ["); print(buffer); print("]\n");
	insw(data_port, buffer, n_bytes >> 1);
	// print("after: buffer = ["); print(buffer); print("]\n");

	SHOW_DISK_CTRL_STATUS("STATUS [after insb] STATUS:");
	SHOW_DISK_CTRL_ERROR("ERROR [after insb] ERROR:");

	print("Finished reading from disk.\n");
	enable_interrupts();
	return NONE;
}

enum sys_error write_to_disk(enum disk_channel channel, enum drive_class class, lba_t block_address, int n_bytes, void* buffer) {
	uint16_t drive_select_port;
	uint16_t sector_count_port;
	uint16_t lba_high_port;
	uint16_t lba_low_port;
	uint16_t lba_mid_port;
	uint16_t command_port;
	uint16_t status_port;
	uint16_t error_port;
	uint16_t data_port;
	int write_command;
	int num_sectors;

	if (n_bytes <= 0) {
		return -1;
	}

	print("Writing to Disk...\n");
	disable_interrupts();
	assign_ports(channel,
							 &drive_select_port,
							 &sector_count_port,
							 &lba_high_port,
							 &lba_low_port,
							 &lba_mid_port,
							 &command_port,
							 &status_port,
							 &error_port,
							 &data_port);

	num_sectors = (n_bytes >> 9) + (n_bytes & 0x1ff ? 1 : 0);

	while((port_byte_in(status_port) & 0xc0) != 0x40) {
		int hd_ctrl_status = port_byte_in(status_port);
		int_to_string(tmp, hd_ctrl_status, 9);
		// print("hd_ctrl_status:"); print(tmp); print("\n");
		// print("Disk Controller busy...\n");
	}

	port_byte_out(error_port, 0x0);
	port_byte_out(sector_count_port, num_sectors);
	port_byte_out(lba_low_port, block_address);
	port_byte_out(lba_mid_port, block_address >> 8);
	port_byte_out(lba_high_port, block_address >> 16);
	port_byte_out(drive_select_port, (0xE0 | (class == SLAVE ? 0x10 : 0x0) | ((block_address >> 24) & 0x0f)));

	SHOW_DISK_CTRL_STATUS("STATUS [before write] STATUS:");
	SHOW_DISK_CTRL_ERROR("ERROR [before write] ERROR:");

	while ((hd_ctrl_status & 0xc0) != 0x40) { // Loop while controller is busy nor not ready.
		int_to_string(tmp, hd_ctrl_status, 9);
		// print("HD BUSY! STATUS:"); print(tmp); print("\n");
		hd_ctrl_status = port_byte_in(status_port);
	}

	// Send read command to controller.
	write_command = num_sectors > 1 ? HD_WRITE_MULTIPLE : HD_WRITE;

	port_byte_out(command_port, write_command);

	SHOW_DISK_CTRL_STATUS("STATUS [after write (1)] STATUS:");
	SHOW_DISK_CTRL_ERROR("ERROR [after write (1)] ERROR:");

	SHOW_DISK_CTRL_STATUS("STATUS [after write (2)] STATUS:");
	SHOW_DISK_CTRL_ERROR("ERROR [after write (2)] ERROR:");

	while ((hd_ctrl_status & 0xc0) != 0x40) { // Loop while controller is busy nor not ready.
		int_to_string(tmp, hd_ctrl_status, 9);
		print("HD BUSY! STATUS:"); print(tmp); print("\n");
		hd_ctrl_status = port_byte_in(status_port);
	}

	int_to_string(tmp, hd_ctrl_status, 9);
	// print("HD READY! STATUS:"); print(tmp); print("\n");

	outsw(data_port, buffer, n_bytes >> 1);

	SHOW_DISK_CTRL_STATUS("STATUS [after outsw] STATUS:");
	SHOW_DISK_CTRL_ERROR("ERROR [after outsw] ERROR:");

	print("Finished writing to disk.\n");
	enable_interrupts();
	return NONE;
}

static void assign_ports(enum disk_channel channel, uint16_t* drive_select_port,
	uint16_t* sector_count_port, uint16_t* lba_high_port, uint16_t* lba_low_port,
	uint16_t* lba_mid_port, uint16_t* command_port, uint16_t* status_port,
	uint16_t* error_port, uint16_t* data_port) {
	
	if (channel == PRIMARY) {
		// print("primary drive selected\n");
		*sector_count_port = HD_PORT_SECT_COUNT_PRIMARY;
		*drive_select_port = HD_PORT_DRV_HEAD_PRIMARY;
		*lba_high_port = HD_PORT_LBA_HIGH_PRIMARY;
		*lba_low_port = HD_PORT_LBA_LOW_PRIMARY;
		*lba_mid_port = HD_PORT_LBA_MID_PRIMARY;
		*command_port = HD_PORT_COMMAND_PRIMARY;
		*status_port = HD_PORT_STATUS_PRIMARY;
		*error_port = HD_PORT_ERROR_PRIMARY;
		*data_port = HD_PORT_DATA_PRIMARY;
	} else if (channel == SECONDARY) {
		print("secondary drive selected");
		*sector_count_port = HD_PORT_SECT_COUNT_SECONDARY;
		*drive_select_port = HD_PORT_DRV_HEAD_SECONDARY;
		*lba_high_port = HD_PORT_LBA_HIGH_SECONDARY;
		*lba_low_port = HD_PORT_LBA_LOW_SECONDARY;
		*lba_mid_port = HD_PORT_LBA_MID_SECONDARY;
		*command_port = HD_PORT_COMMAND_SECONDARY;
		*status_port = HD_PORT_STATUS_SECONDARY;
		*error_port = HD_PORT_ERROR_SECONDARY;
		*data_port = HD_PORT_DATA_SECONDARY;
	} else {
		print("Bad drive selection. Neither primary or secondary chosen.\n");
	}
}

void lba_to_hd_sect_params(unsigned int lba, struct hd_sect_params* hd_params) {
	unsigned int heads_per_cylinder = 16, sectors_per_cylinder = 63;
	hd_params->cylinder = lba / (heads_per_cylinder * sectors_per_cylinder);
	hd_params->head = (lba / sectors_per_cylinder) % heads_per_cylinder;
	hd_params->sector = (lba % sectors_per_cylinder) + 1;
}
