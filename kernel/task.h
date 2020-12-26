#ifndef __TASK_H__

#include "system.h"

// Note: Fields ending in `_{l,u}16b` only use 16 bits.
// If such a field is defined with `unsigned int`, 
// it occupies the upper (u) or lower (l) 16 bits.
struct task_state_segment {                 /* Setting? */
    unsigned int previous_task_link_l16b;   /* Y */
    unsigned int ESP0;                      /* Y */
    unsigned int SS0_l16b;                  /* Y */
    unsigned int ESP1;
    unsigned int SS1_l16b;
    unsigned int ESP2;
    unsigned int SS2_l16b;
    unsigned int CR3;                       /* Y */
    unsigned int EIP;                       /* Y */
    unsigned int EFLAGS;  
    unsigned int EAX; 
    unsigned int ECX; 
    unsigned int EDX; 
    unsigned int EBX; 
    unsigned int ESP; 
    unsigned int EBP; 
    unsigned int ESI; 
    unsigned int EDI; 
    unsigned int ES_l16b;                   /* Y */
    unsigned int CS_l16b;                   /* Y */
    unsigned int SS_l16b;                   /* Y */
    unsigned int DS_l16b;                   /* Y */
    unsigned int FS_l16b;                   /* Y */
    unsigned int GS_l16b;                   /* Y */
    unsigned int LDT_u16b;
    unsigned int SSP; 
}__attribute__((packed));

typedef struct task_state_segment tss;

void setup_tss();

void do_task_switch();

#endif // __TASK_H__
