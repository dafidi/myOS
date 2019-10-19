#include "system.h"

void int_to_string(char* s, int val, int n) {
  const int size = n;
  char t;
  for (int i = 0; i < size && val; i++) {
    t = val % 10;
    s[size - i - 1] = t + 48;
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
