#ifndef __STRING_H__
#define __STRING_H__

#include <kernel/system.h>

#define STR_MESSAGE_LENGTH 256

void int_to_string(char* s, unsigned  int val, int n);

bool strmatchn(char* s1, char* s2, int n);

void strcopy(char* dest, const char* src);

bool strmatchn(char* s1, char* s2, int n);

int strlen(char* str);

void set_bit(uint8_t* addr, int nr);

#endif
