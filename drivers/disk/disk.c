#include "disk.h"

#include "kernel/low_level.h"
#include "drivers/screen/screen.h"
#include "kernel/system.h"

int hd_addr = 1 << 28;
char buf[1024] = "BEFORE";
char bigtmp[12] = "00000000000";
char tmp[10] = "0000000";

void reset_hd(void) {
  unsigned short hd_status;
  while((port_byte_in(HD_PORT_STATUS) & 0xc0) != 0x40) {
    print("Disk Controller busy...\n");
  }
  
  port_byte_out(HD_PORT_COMMAND, 0x08); 
}

void read_from_disk(){
  unsigned short hd_status;
  print("Reading from Disk...\n");
  while((port_byte_in(HD_PORT_STATUS) & 0xc0) != 0x40) {
    print("Disk Controller busy...\n");
  }

  port_byte_out(HD_PORT_ERROR, 0x0);
  port_byte_out(HD_PORT_SECT_COUNT, 1);
  port_byte_out(HD_PORT_SECT_NUM, (unsigned short) hd_addr);
  port_byte_out(HD_PORT_CYL_LOW, (unsigned short) (hd_addr >> 8));
  port_byte_out(HD_PORT_CYL_HIGH, (unsigned short) (hd_addr >> 16));
  port_byte_out(HD_PORT_DRV_HEAD, (0xE0 | ((hd_addr >> 24) & 0x0f)));

  while(hd_status & 0x80) {
    hd_status = port_byte_in(HD_PORT_STATUS);
    int_to_string(tmp, hd_status, 9);
    print("Disk Controller busy...:");print(tmp);print("\n");
  } 
  port_byte_out(HD_PORT_COMMAND, HD_READ); 
 
  while ((port_byte_in(HD_PORT_STATUS)&0x80)) {
    print("HD BUSY!\n");
  }

  print("before:\n[");
  print(buf);
  print("]\n");
  insl(HD_PORT_DATA, buf, 3);
  print("after:\n[");
  print(buf);
  print("]\n");

  print("Done reading from disk.\n");
}

void lba_to_hd_sect_params(unsigned int lba, struct hd_sect_params* hd_params) {
  unsigned int heads_per_cylinder = 16, sectors_per_cylinder = 63;
  hd_params->cylinder = lba / (heads_per_cylinder * sectors_per_cylinder);
  hd_params->head = (lba / sectors_per_cylinder) % heads_per_cylinder;
  hd_params->sector = (lba % sectors_per_cylinder) + 1;
}

