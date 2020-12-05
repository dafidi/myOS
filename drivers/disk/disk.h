#ifndef __DISK_H__
#define __DISK_H__

#include <kernel/system.h>
#include <kernel/error.h>

#define HD_PORT_DATA_PRIMARY        0x1f0
#define HD_PORT_ERROR_PRIMARY       0x1f1
#define HD_PORT_SECT_COUNT_PRIMARY  0x1f2
#define HD_PORT_LBA_LOW_PRIMARY    0x1f3
#define HD_PORT_LBA_MID_PRIMARY     0x1f4
#define HD_PORT_LBA_HIGH_PRIMARY    0x1f5
#define HD_PORT_DRV_HEAD_PRIMARY    0x1f6
#define HD_PORT_STATUS_PRIMARY      0x1f7
#define HD_PORT_COMMAND_PRIMARY     0x1f7 

#define HD_PORT_DATA_SECONDARY        0x170
#define HD_PORT_ERROR_SECONDARY       0x171
#define HD_PORT_SECT_COUNT_SECONDARY  0x172
#define HD_PORT_LBA_LOW_SECONDARY    0x173
#define HD_PORT_LBA_MID_SECONDARY     0x174
#define HD_PORT_LBA_HIGH_SECONDARY    0x175
#define HD_PORT_DRV_HEAD_SECONDARY    0x176
#define HD_PORT_STATUS_SECONDARY      0x177
#define HD_PORT_COMMAND_SECONDARY     0x177 

#define HD_PRIMARY

#ifdef HD_PRIMARY
#define HD_PORT_DATA        HD_PORT_DATA_PRIMARY
#define HD_PORT_ERROR       HD_PORT_ERROR_PRIMARY
#define HD_PORT_SECT_COUNT  HD_PORT_SECT_COUNT_PRIMARY
#define HD_PORT_LBA_LOW    HD_PORT_LBA_LOW_PRIMARY
#define HD_PORT_LBA_MID     HD_PORT_LBA_MID_PRIMARY
#define HD_PORT_LBA_HIGH    HD_PORT_LBA_HIGH_PRIMARY
#define HD_PORT_DRV_HEAD    HD_PORT_DRV_HEAD_PRIMARY
#define HD_PORT_STATUS      HD_PORT_STATUS_PRIMARY
#define HD_PORT_COMMAND     HD_PORT_COMMAND_PRIMARY
#else 
#define HD_PORT_DATA        HD_PORT_DATA_SECONDARY
#define HD_PORT_ERROR       HD_PORT_ERROR_SECONDARY
#define HD_PORT_SECT_COUNT  HD_PORT_SECT_COUNT_SECONDARY
#define HD_PORT_LBA_LOW    HD_PORT_LBA_LOW_SECONDARY
#define HD_PORT_LBA_MID     HD_PORT_LBA_MID_SECONDARY
#define HD_PORT_LBA_HIGH    HD_PORT_LBA_HIGH_SECONDARY
#define HD_PORT_DRV_HEAD    HD_PORT_DRV_HEAD_SECONDARY
#define HD_PORT_STATUS      HD_PORT_STATUS_SECONDARY
#define HD_PORT_COMMAND     HD_PORT_COMMAND_SECONDARY
#endif

// IDE/ATA PIO Commands.
#define HD_READ                  0x20
#define HD_READ_MULTIPLE         0xC4
#define HD_WRITE                 0x30
#define HD_WRITE_MULTIPLE        0xC5

typedef uint32_t lba_t;

enum disk_channel {
	PRIMARY,
	SECONDARY 
};

enum drive_class {
	MASTER,
	SLAVE
};

void init_disk(void);
void install_disk_irq_handler(void);
void disk_irq_handler(struct registers* r);

void read_from_storage_disk(lba_t, int, void*);
void write_to_storage_disk(lba_t, int, void*);

enum sys_error read_from_disk(enum disk_channel, enum drive_class, lba_t, int, void* buffer);
enum sys_error write_to_disk(enum disk_channel, enum drive_class, lba_t, int, void* buffer);

// CHS stuff (not used).
struct hd_sect_params {
	unsigned int cylinder;
	unsigned int head;
	unsigned int sector;
};

void lba_to_hd_sect_params(unsigned int, struct hd_sect_params*);

#define HD_PORT_SECT_NUM_PRIMARY    0x1f3
#define HD_PORT_CYL_LOW_PRIMARY     0x1f4
#define HD_PORT_CYL_HIGH_PRIMARY    0x1f5

#endif /* __DISK_H__ */
