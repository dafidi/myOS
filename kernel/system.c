#include "system.h"

#include "low_level.h"

/* Copy  bytes  from  one  place  to  another. */
void memcpy(char* dest, const char* source, int no_bytes) {
    uint64_t i = 0ULL;

    for (i=0; i<no_bytes; i++) {
        *(dest + i) = *( source + i);
    }
}

void *memset(void* src, int c, unsigned long n) {
    char* ssrc = src;
    for (int i = 0; i < n; i++) {
        ssrc[i] = c;
    }
    return src;
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

void fill_long_buffer(unsigned int *buffer, const int start_index, int num_entries, const unsigned long val) {
    const int end_index = start_index + num_entries;
    int i;

    for (i = start_index; i < end_index; i++) {
        buffer[i] = val;
    }
}

uint64_t udiv(uint64_t n, uint64_t d) {
    uint64_t q = 0;
    uint64_t r = 0;
    int i = 0;

    for (i = 63; i >= 0; i--) {
        r = r << 1;
        r = r | ((n >> i) & 1);
        if (r >= d) {
            r = r - d;
            q = q | (1 << i);
        }
    }

    return q;
}

/**
 * This very incorrectly named function is supposed to 
 * 
 */
uint64_t round_to_next_quartet(uint64_t val) {
    uint64_t next_rounded_quartet;
    int qidx, idx;

    // Find most significant set bit.
    idx = bit_scan_reverse64(val);

    // Identify which quartet it is in.
    qidx = idx / 4;

    // Isolate the value of that quartet.
    next_rounded_quartet = val >> (qidx * 4);

    // Increment to get the next quartet's value.
    next_rounded_quartet++;

    // Shift left to restore original quartet index.
    next_rounded_quartet <<= (qidx * 4);

    return next_rounded_quartet;
}
