#ifndef __TASK_H__
#define __TASK_H__

#include "mm/mm.h"
#include "system.h"

// Note: Fields ending in `_{l,u}16b` only use 16 bits.
// If such a field is defined with `unsigned int`, 
// it occupies the upper (u) or lower (l) 16 bits.
typedef struct task_state_segment {                 /* Setting? */
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
}__attribute__((packed)) tss_t;

typedef struct task_state_segment64 {                 /* Setting? */
        uint32_t reserved0;
        uint32_t rsp0l;
        uint32_t rsp0h;
        uint32_t rsp1l;
        uint32_t rsp1h;
        uint32_t rsp2l;
        uint32_t rsp2h;
        uint32_t reserved1;
        uint32_t reserved2;
        uint32_t ist1l;
        uint32_t ist1h;
        uint32_t ist2l;
        uint32_t ist2h;
        uint32_t ist3l;
        uint32_t ist3h;
        uint32_t ist4l;
        uint32_t ist4h;
        uint32_t ist5l;
        uint32_t ist5h;
        uint32_t ist6l;
        uint32_t ist6h;
        uint32_t ist7l;
        uint32_t ist7h;
        uint32_t reserved3;
        uint32_t reserved4;
        uint16_t reserved5;
        uint16_t io_map_base_addr;
}__attribute__((packed)) tss64_t;

typedef struct task_info {
    /* Virtual address the program expects to start from. */
    va_t start_virt_addr;
    /* Physical address of program in memory. */
    pa_t start_phy_addr;
    /* Typically the size of the .text section of the program. */
    va_range_sz_t mem_required;
    /* Max size of heap allocated to program. */
    va_range_sz_t heap_size;
    /* Max size of stack allocated to program. */
    va_range_sz_t stack_size;
    /* Info on finding the program in disk.*/
    unsigned long start_disk_sector;

}__attribute__((packed)) task_info;

void setup_tss(void);

void exec_waiting_tasks(void);
void exec_task(struct task_info *task);

void init_task_system(void);

#endif // __TASK_H__
