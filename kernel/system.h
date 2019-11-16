#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#define DEFAULT_TIMER_FREQUENCY_HZ 100
#define STR_MESSAGE_LENGTH 256

struct registers {
  unsigned int gs, fs, es, ds;      /* pushed the segs last */
  unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;  /* pushed by 'pusha' */
  unsigned int int_no, err_code;    /* our 'push byte #' and ecodes do this */
  unsigned int eip, cs, eflags, useresp, ss;   /* pushed by the processor automatically */ 
}__attribute__((packed));

void int_to_string(char* s, int val, int n);

void strcopy(char* dest, const char* src);

extern void enable_interrupts(void);
extern void disable_interrupts(void);

#endif /* __SYSTEM_H__ */
