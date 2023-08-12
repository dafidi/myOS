.code32

.globl mem_map_buf_addr
.globl mem_map_buf_entry_count
.globl max_kernel_static_mem
.globl kernel_entry

.extern main

.align 2
# kernel_entry expects the following information about the
# BIOS's memory map to be put on the stack:
#   the address of the buffer holding the memory map (top of stack)
#   the number of entries in the memory map.
kernel_entry:
    movl (%esp), %eax
    movl %eax, max_kernel_static_mem(,1)
    movl 4(%esp), %eax
    movl %eax, mem_map_buf_addr(,1)
    movl 8(%esp), %eax
    movl %eax, mem_map_buf_entry_count(,1)

.ifndef CONFIG32
    call init_64bit_mode
.code64
.endif

    call main

.set KERNEL_TASK_SEG_IDX,  5
.set KERNEL_TASK_SEG, 8 * KERNEL_TASK_SEG_IDX

# Function to load kernel task register.
.globl load_kernel_tr
load_kernel_tr:
    ltr kernel_task_selector(%rip)
    ret

.include "kernel/asm/kernel64.s"

.data
kernel_task_selector: .long KERNEL_TASK_SEG

.ifdef CONFIG32
#mem_map_buf_addr: .long 0x0
#dynamic_memory_start: .long 0x0
.else
max_kernel_static_mem: .quad 0x0
mem_map_buf_addr: .quad 0x0
.endif
mem_map_buf_entry_count: .long 0x0

