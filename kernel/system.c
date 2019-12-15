#include "system.h"

void clear_buffer(char* buffer, int n) {
  int i;
  for (i = 0; i < n; i++) {
    buffer[i] = (char) 0;
  }
}
