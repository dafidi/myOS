#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#define SYSTEM_RAM_BYTES (0x1ULL << 32)

#define DEFAULT_TIMER_FREQUENCY_HZ 100

// TODO: figure out a way to share these across C and .asm files.
enum GDT_ENTRY_IDX {
	SYSTEM_GDT_KERNEL_CODE_IDX = 1,
	SYSTEM_GDT_KERNEL_DATA_IDX,
	SYSTEM_GDT_USER_CODE_IDX,
	SYSTEM_GDT_USER_DATA_IDX,
	SYSTEM_KERNEL_TSS_DESCRIPTOR_IDX,
	SYSTEM_USER_TSS_DESCRIPTOR_IDX
};

#define bool unsigned char
#define true 1
#define false 0

#define PAUSE() while(true)

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

struct registers {
	unsigned int gs, fs, es, ds;      /* pushed the segs last */
	unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;  /* pushed by 'pusha' */
	unsigned int int_no, err_code;    /* our 'push byte #' and ecodes do this */
	unsigned int eip, cs, eflags, useresp, ss;   /* pushed by the processor automatically */ 
}__attribute__((packed));

void  memory_copy(char* source , char* dest , int  no_bytes);
void clear_buffer(uint8_t* buffer, int n);
void fill_byte_buffer(unsigned char *buffer, const int start_index, int num_entries, const unsigned char val);
void fill_word_buffer(unsigned short *buffer, const int start_index, int num_entries, const unsigned short val);
void fill_long_buffer(unsigned long *buffer, const int start_index, int num_entries, const unsigned long val);

extern void enable_interrupts(void);
extern void disable_interrupts(void);

#endif /* __SYSTEM_H__ */
