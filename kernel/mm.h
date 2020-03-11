#ifndef __MM_H__

#include "system.h"

struct bios_mem_map {
  unsigned long long base;
  unsigned long long length;
  unsigned int type;
  unsigned int acpi_null;
}__attribute__((packed));

void init_mm(void);
#endif // __MM_H__
