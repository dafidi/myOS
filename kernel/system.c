#include "system.h"

void clear_buffer(uint8_t* buffer, int n) {
	int i;
	for (i = 0; i < n; i++) {
		buffer[i] = (char) 0;
	}
}
