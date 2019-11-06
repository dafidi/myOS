#ifndef __DISK_H__
#define __DISK_H__

#define HD_PORT_DATA_PRIMARY        0x1f0
#define HD_PORT_ERROR_PRIMARY       0x1f1
#define HD_PORT_SECT_COUNT_PRIMARY  0x1f2
#define HD_PORT_SECT_NUM_PRIMARY    0x1f3
#define HD_PORT_CYL_LOW_PRIMARY     0x1f4
#define HD_PORT_CYL_HIGH_PRIMARY    0x1f5
#define HD_PORT_DRV_HEAD_PRIMARY    0x1f6
#define HD_PORT_STATUS_PRIMARY      0x1f7
#define HD_PORT_COMMAND_PRIMARY     0x1f7 

#define HD_PORT_DATA_SECONDARY        0x170
#define HD_PORT_ERROR_SECONDARY       0x171
#define HD_PORT_SECT_COUNT_SECONDARY  0x172
#define HD_PORT_SECT_NUM_SECONDARY    0x173
#define HD_PORT_CYL_LOW_SECONDARY     0x174
#define HD_PORT_CYL_HIGH_SECONDARY    0x175
#define HD_PORT_DRV_HEAD_SECONDARY    0x176
#define HD_PORT_STATUS_SECONDARY      0x177
#define HD_PORT_COMMAND_SECONDARY     0x177 

#define HD_PRIMARY

#ifdef HD_PRIMARY

#define HD_PORT_DATA        HD_PORT_DATA_PRIMARY
#define HD_PORT_ERROR       HD_PORT_ERROR_PRIMARY
#define HD_PORT_SECT_COUNT  HD_PORT_SECT_COUNT_PRIMARY
#define HD_PORT_SECT_NUM    HD_PORT_SECT_NUM_PRIMARY
#define HD_PORT_CYL_LOW     HD_PORT_CYL_LOW_PRIMARY
#define HD_PORT_CYL_HIGH    HD_PORT_CYL_HIGH_PRIMARY
#define HD_PORT_DRV_HEAD    HD_PORT_DRV_HEAD_PRIMARY
#define HD_PORT_STATUS      HD_PORT_STATUS_PRIMARY
#define HD_PORT_COMMAND     HD_PORT_COMMAND_PRIMARY

#else 

#define HD_PORT_DATA            HD_PORT_DATA_SECONDARY
#define HD_PORT_ERROR           HD_PORT_ERROR_SECONDARY
#define HD_PORT_SECT_COUNT      HD_PORT_SECT_COUNT_SECONDARY
#define HD_PORT_SECT_NUM        HD_PORT_SECT_NUM_SECONDARY
#define HD_PORT_CYL_LOW         HD_PORT_CYL_LOW_SECONDARY
#define HD_PORT_CYL_HIGH        HD_PORT_CYL_HIGH_SECONDARY
#define HD_PORT_DRV_HEAD        HD_PORT_DRV_HEAD_SECONDARY
#define HD_PORT_STATUS          HD_PORT_STATUS_SECONDARY
#define HD_PORT_COMMAND         HD_PORT_COMMAND_SECONDARY

#endif

#define HD_READ         0x20
#define HD_WRITE        0x30

struct hd_sect_params {
  unsigned int cylinder;
  unsigned int head;
  unsigned int sector;
};

void reset_hd(void);
void read_from_disk(void);
void lba_to_hd_sect_params(unsigned int, struct hd_sect_params*);

#endif /* __DISK_H__ */
