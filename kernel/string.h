#ifndef __STRING_H__
#define __STRING_H__

#include "system.h"

#define STR_MESSAGE_LENGTH 256
#define NULL ((void*) 0)

void int_to_string(char* s, unsigned  int val, int n);

bool strmatchn(char* s1, char* s2, int n);

void strcopy(char* dest, const char* src);

bool strmatchn(char* s1, char* s2, int n);

int strlen(char* str);

void set_bit(uint8_t* addr, const int nr);
void clear_bit(uint8_t* addr, const int nr);
unsigned char get_bit(const uint8_t* addr, const int nr);

#endif
