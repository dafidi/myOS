#include "system.h"

/* Copy  bytes  from  one  place  to  another. */
void  memory_copy(const char* source , char* dest , int  no_bytes) {
    int i;
    for (i=0; i<no_bytes; i++) {
        *(dest + i) = *( source + i);
    }
}

void clear_buffer(uint8_t* buffer, int n) {
    int i;

    for (i = 0; i < n; i++) {
        buffer[i] = (char) 0;
    }
}

void fill_byte_buffer(unsigned char *buffer, const int start_index, int num_entries, const unsigned char val) {
    const int end_index = start_index + num_entries;
    int i;

    for (i = start_index; i < end_index; i++) {
        buffer[i] = val;
    }
}

void fill_short_buffer(unsigned short *buffer, const int start_index, int num_entries, const unsigned short val) {
    const int end_index = start_index + num_entries;
    int i;

    for (i = start_index; i < end_index; i++) {
        buffer[i] = val;
    }
}

void fill_long_buffer(unsigned long *buffer, const int start_index, int num_entries, const unsigned long val) {
    const int end_index = start_index + num_entries;
    int i;

    for (i = start_index; i < end_index; i++) {
        buffer[i] = val;
    }
}
