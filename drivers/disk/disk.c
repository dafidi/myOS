#include "disk.h"

#include <drivers/screen/screen.h>
#include <kernel/irq.h>
#include <kernel/low_level.h>

// (35 * 512 + 512) / 512 Calculation logic:
// Assuming the boot sector and kernel take up 36 sectors/blocks (indexes 0-35)
// the block_num_to_read should be at index 36.
static int block_num_to_read = (35 * 512 + 512) / 512;
static char buffer[1024] = "BEFORE";
static char tmp[10] = "0000000";

static unsigned char hd_ctrl_status;
static unsigned char hd_ctrl_error;

#define LOGGING_ENABLED
#if defined(LOGGING_ENABLED)
#define SHOW_DISK_CTRL_STATUS(msg) hd_ctrl_status = port_byte_in(HD_PORT_STATUS_PRIMARY); \
                                   int_to_string(tmp, hd_ctrl_status, 9); \
                                   print(msg); print(tmp); print("\n");

#define SHOW_DISK_CTRL_ERROR(msg) hd_ctrl_error = port_byte_in(HD_PORT_ERROR_PRIMARY); \
                                   int_to_string(tmp, hd_ctrl_error, 9); \
                                   print(msg); print(tmp); print("\n");
#else
#define SHOW_DISK_CTRL_STATUS(msg)
#define SHOW_DISK_CTRL_ERROR(msg)
#endif

// void disk_irq_handler(struct registers* r) {
//   print("I am IRQ 14, the chosen one.\n");
//   // insw(HD_PORT_DATA_PRIMARY, buffer, 10);
//   char* tbuffer = buffer ;
//   print(tbuffer); print("\n");
//   SHOW_DISK_CTRL_STATUS("STATUS [after insb] STATUS:");
// }

// void install_disk_irq_handler(void) {
//   // install_irq(14, disk_irq_handler);
// }

// Read from ATA/IDE hard disk into buffer. Currently this program simply
// Reads 32 bytes (16 words) from the point on the disk past the kernel.
void read_from_disk(void) {
  print("Reading from Disk...\n");
	disable_interrupts();

  while((port_byte_in(HD_PORT_STATUS_PRIMARY) & 0xc0) != 0x40) {
    print("Disk Controller busy...\n");
  }

  port_byte_out(HD_PORT_ERROR_PRIMARY, 0x0);
  port_byte_out(HD_PORT_SECT_COUNT_PRIMARY, 3);
  port_byte_out(HD_PORT_LBA_LOW_PRIMARY, block_num_to_read);
  port_byte_out(HD_PORT_LBA_MID_PRIMARY, block_num_to_read >> 8);
  port_byte_out(HD_PORT_LBA_HIGH_PRIMARY, block_num_to_read >> 16);
  port_byte_out(HD_PORT_DRV_HEAD_PRIMARY, (0xE0 | ((block_num_to_read >> 24) & 0x0f)));

  SHOW_DISK_CTRL_STATUS("STATUS [before read] STATUS:");
  SHOW_DISK_CTRL_ERROR("ERROR [before read] ERROR:");

  while ((hd_ctrl_status & 0xc0) != 0x40) { // Loop while controller is busy nor not ready.
    int_to_string(tmp, hd_ctrl_status, 9);
    print("HD BUSY! STATUS:"); print(tmp); print("\n");
    hd_ctrl_status = port_byte_in(HD_PORT_STATUS_PRIMARY);
  }

  // Send read command to controller.
  port_byte_out(HD_PORT_COMMAND_PRIMARY, HD_READ);

  SHOW_DISK_CTRL_STATUS("STATUS [after read (1)] STATUS:");
  SHOW_DISK_CTRL_ERROR("ERROR [after read (1)] ERROR:");

  SHOW_DISK_CTRL_STATUS("STATUS [after read (2)] STATUS:");
  SHOW_DISK_CTRL_ERROR("ERROR [after read (2)] ERROR:");

  while ((hd_ctrl_status & 0xc0) != 0x40) { // Loop while controller is busy nor not ready.
    int_to_string(tmp, hd_ctrl_status, 9);
    print("HD BUSY! STATUS:"); print(tmp); print("\n");
    hd_ctrl_status = port_byte_in(HD_PORT_STATUS_PRIMARY);
  }

  int_to_string(tmp, hd_ctrl_status, 9);
  print("HD READY! STATUS:"); print(tmp); print("\n");

  print("before:\n[");
  print(buffer);
  print("]\n");
  insw(HD_PORT_DATA_PRIMARY, buffer, 16);
  print("after:\n[");
  print(buffer);
  print("]\n");

  SHOW_DISK_CTRL_STATUS("STATUS [after insb] STATUS:");
  SHOW_DISK_CTRL_ERROR("ERROR [after insb] ERROR:");

  print("Done reading from disk.\n");
	enable_interrupts();
  return;
}

void lba_to_hd_sect_params(unsigned int lba, struct hd_sect_params* hd_params) {
  unsigned int heads_per_cylinder = 16, sectors_per_cylinder = 63;
  hd_params->cylinder = lba / (heads_per_cylinder * sectors_per_cylinder);
  hd_params->head = (lba / sectors_per_cylinder) % heads_per_cylinder;
  hd_params->sector = (lba % sectors_per_cylinder) + 1;
}
