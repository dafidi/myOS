#ifndef __PRINT_H__
#define __PRINT_H__

#include "system.h"

void print_registers(struct registers *registers);
void print_registers64(struct registers64 *registers);
void print_string(const char* message);
void print_int32(int n);
void print_int64(int n);
void print_ptr(const void *p);
void print_uint(unsigned long long int n);

#endif
