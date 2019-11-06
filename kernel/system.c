#include "system.h"

void int_to_string(char* s, int val, int n) {
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
