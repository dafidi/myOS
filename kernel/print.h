#ifndef __PRINT_H__
#define __PRINT_H__

#include "system.h"

void print_registers(struct registers *registers);
void print_string(const char* message);
void print_int32(int n);
void print_ptr(void *p);

#endif
