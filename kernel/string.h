#ifndef __STRING_H__
#define __STRING_H__

#include <kernel/system.h>

#define STR_MESSAGE_LENGTH 256

void int_to_string(char* s, int val, int n);

bool strmatchn(char* s1, char* s2, int n);

void strcopy(char* dest, const char* src);

int strlen(char* str);

void print_int32(int);

void set_bit(char* addr, int nr);

#endif
