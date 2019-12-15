#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#define DEFAULT_TIMER_FREQUENCY_HZ 100

#define bool unsigned char
#define true 1
#define false 0

typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

struct registers {
  unsigned int gs, fs, es, ds;      /* pushed the segs last */
  unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;  /* pushed by 'pusha' */
  unsigned int int_no, err_code;    /* our 'push byte #' and ecodes do this */
  unsigned int eip, cs, eflags, useresp, ss;   /* pushed by the processor automatically */ 
}__attribute__((packed));

void clear_buffer(char* buffer, int n);

extern void enable_interrupts(void);
extern void disable_interrupts(void);


#endif /* __SYSTEM_H__ */
