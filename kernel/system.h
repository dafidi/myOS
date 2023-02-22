#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#define PAGE_SIZE_SHIFT 12
#define PAGE_SIZE (1 << PAGE_SIZE_SHIFT)
#define SYSTEM_RAM_BYTES (0x1ULL << 32)
#define SYSTEM_NUM_PAGES (SYSTEM_RAM_BYTES >> PAGE_SIZE_SHIFT)

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
#define INFINITE_LOOP() while(true)

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef long long unsigned int uint64_t;

struct registers {
    unsigned int gs, fs, es, ds;      /* pushed the segs last */
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;  /* pushed by 'pusha' */
    unsigned int int_no, err_code;    /* our 'push byte #' and ecodes do this */
    unsigned int eip, cs, eflags, useresp, ss;   /* pushed by the processor automatically */ 
}__attribute__((packed));

struct registers64 {
    uint64_t rdi, rsi, rbp, rsp, rbx, rdx, rcx, rax;  /* pushed by asm handler */
    uint64_t int_no;    /* our 'push byte #' and ecodes do this */
    uint64_t err_code, rip, cs, rflags, userrsp, ss;   /* pushed by the processor automatically */ 
}__attribute__((packed));

void  memory_copy(const char* source , char* dest , int  no_bytes);
void clear_buffer(uint8_t* buffer, int n);
void fill_byte_buffer(unsigned char *buffer, const int start_index, int num_entries, const unsigned char val);
void fill_word_buffer(unsigned short *buffer, const int start_index, int num_entries, const unsigned short val);
void fill_long_buffer(unsigned long *buffer, const int start_index, int num_entries, const unsigned long val);

extern void enable_interrupts(void);
extern void disable_interrupts(void);
extern void enable_interrupts64(void);
extern void disable_interrupts64(void);

// These mainly to try to avoid warnings in gcc.
// gcc will let you do an explicit cast of (uint32_t)(uint64_t)(x).
#define addr_to_u32(x) ((uint32_t)(to_addr_width(x)))
#define addr_to_u64(x) ((uint64_t)(to_addr_width(x)))
#define u32_to_addr(x) (to_addr_width((uint32_t)x))
#define u64_to_addr(x) (to_addr_width((uint64_t)x))
#ifdef CONFIG32
#define to_addr_width(x) ((uint32_t)(x))
#else
#define to_addr_width(x) ((uint64_t)(x))
#endif

#endif /* __SYSTEM_H__ */
