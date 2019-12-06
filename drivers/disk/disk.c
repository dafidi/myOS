#include "disk.h"

#include <drivers/screen/screen.h>
#include <kernel/irq.h>
#include <kernel/low_level.h>

// Data on the boot disk starts form sector index: (35 * 512 + 512) / 512
// Data on the storage disk starts from sector index: 0.
// Choose wisely.
static int block_num_to_read = 0;

// Helpful variables.
static char buffer[1024] = "BEFORE";
static char tmp[10] = "0000000";

static unsigned char hd_ctrl_status;
static unsigned char hd_ctrl_error;

#define LOGGING_ENABLED
#if defined(LOGGING_ENABLED)
#define SHOW_DISK_CTRL_STATUS(msg) hd_ctrl_status = port_byte_in(HD_PORT_STATUS); \
                                   int_to_string(tmp, hd_ctrl_status, 9); \
                                   print(msg); print(tmp); print("\n");

#define SHOW_DISK_CTRL_ERROR(msg) hd_ctrl_error = port_byte_in(HD_PORT_ERROR); \
                                   int_to_string(tmp, hd_ctrl_error, 9); \
                                   print(msg); print(tmp); print("\n");
#else
#define SHOW_DISK_CTRL_STATUS(msg)
#define SHOW_DISK_CTRL_ERROR(msg)
#endif

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
void read_from_storage_disk(void) {
  print("Reading from Disk...\n");
	disable_interrupts();

  while((port_byte_in(HD_PORT_STATUS) & 0xc0) != 0x40) {
    int hd_ctrl_status = port_byte_in(HD_PORT_STATUS);
    int_to_string(tmp, hd_ctrl_status, 9);
    print("hd_ctrl_status:"); print(tmp); print("\n");
    print("Disk Controller busy...\n");
  }

  port_byte_out(HD_PORT_ERROR, 0x0);
  port_byte_out(HD_PORT_SECT_COUNT, 2);
  port_byte_out(HD_PORT_LBA_LOW, block_num_to_read);
  port_byte_out(HD_PORT_LBA_MID, block_num_to_read >> 8);
  port_byte_out(HD_PORT_LBA_HIGH, block_num_to_read >> 16);
  port_byte_out(HD_PORT_DRV_HEAD, (0xF0 | ((block_num_to_read >> 24) & 0x0f)));

  SHOW_DISK_CTRL_STATUS("STATUS [before read] STATUS:");
  SHOW_DISK_CTRL_ERROR("ERROR [before read] ERROR:");

  while ((hd_ctrl_status & 0xc0) != 0x40) { // Loop while controller is busy nor not ready.
    int_to_string(tmp, hd_ctrl_status, 9);
    print("HD BUSY! STATUS:"); print(tmp); print("\n");
    hd_ctrl_status = port_byte_in(HD_PORT_STATUS);
  }

  // Send read command to controller.
  port_byte_out(HD_PORT_COMMAND, HD_READ);

  SHOW_DISK_CTRL_STATUS("STATUS [after read (1)] STATUS:");
  SHOW_DISK_CTRL_ERROR("ERROR [after read (1)] ERROR:");

  SHOW_DISK_CTRL_STATUS("STATUS [after read (2)] STATUS:");
  SHOW_DISK_CTRL_ERROR("ERROR [after read (2)] ERROR:");

  while ((hd_ctrl_status & 0xc0) != 0x40) { // Loop while controller is busy nor not ready.
    int_to_string(tmp, hd_ctrl_status, 9);
    print("HD BUSY! STATUS:"); print(tmp); print("\n");
    hd_ctrl_status = port_byte_in(HD_PORT_STATUS);
  }

  int_to_string(tmp, hd_ctrl_status, 9);
  print("HD READY! STATUS:"); print(tmp); print("\n");

  print("before: ["); print(buffer); print("]\n");
  insw(HD_PORT_DATA, buffer, 512);
  print("after: ["); print(buffer); print("]\n");

  SHOW_DISK_CTRL_STATUS("STATUS [after insb] STATUS:");
  SHOW_DISK_CTRL_ERROR("ERROR [after insb] ERROR:");

  print("Finished reading from disk.\n");
	enable_interrupts();
  return;
}

void lba_to_hd_sect_params(unsigned int lba, struct hd_sect_params* hd_params) {
  unsigned int heads_per_cylinder = 16, sectors_per_cylinder = 63;
  hd_params->cylinder = lba / (heads_per_cylinder * sectors_per_cylinder);
  hd_params->head = (lba / sectors_per_cylinder) % heads_per_cylinder;
  hd_params->sector = (lba % sectors_per_cylinder) + 1;
}
