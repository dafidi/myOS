#include "string.h"

#include "drivers/screen/screen.h"
/*
2^32 = 4294967296
*/
static char int_template[11] = "0000000000";

void int_to_string(char* s, unsigned int val, int n) {
  char t;
  int i;
  
  for (i = 0; i < n; i++) { s[i] = 48; } // Clear vestigial digits.

  for (i = 0; i < n && val; i++) {
    t = val % 10;
    s[n - i - 1] = t + 48;
    val /= 10;
  }

  return;
}

void strcopy(char* dest, const char* src) {
  short curr_index = 0;
  char curr_char = src[curr_index];
  
  while (curr_char && curr_index < STR_MESSAGE_LENGTH) {
    dest[curr_index] = curr_char;
    curr_index += 1;
    curr_char = src[curr_index];
  }
  dest[STR_MESSAGE_LENGTH - 1] = '\0';

  return;
}

bool strmatchn(char* s1, char* s2, int n) {
  bool match = true;
  int i = 0;

  for (i = 0; i < n && match ; i++) {
    match = s1[i] == s2[i]; 
  }

  return match;
}

int strlen(char* str) {
  int i = 0;
  while (str[i] != 0) {
    i++;
  }
  return i;
}

void print_int32(int n) {
  int_to_string(int_template, n, 10);
  print_int(int_template);
}

void set_bit(uint8_t* addr, int nr) {
  int byte_offset = nr / 8;
  uint8_t bit_offset_mod = nr % 8;
  uint8_t c = *(addr + byte_offset);
  uint8_t sh =  0x80 >> bit_offset_mod;
 
  c |= (sh);
  *(addr + byte_offset) = c;
}

