[bits 32]

global mem_map_buf_addr
global mem_map_buf_entry_count
global kernel_entry

extern main

; kernel_entry expects the following information about the
; BIOS's memory map to be put on the stack:
;   the address of the buffer holding the memory map (top of stack)
;   the number of entries in the memory map.
kernel_entry:
    mov eax, [esp]
    mov [mem_map_buf_addr], eax
    mov eax, [esp+4]
    mov [mem_map_buf_entry_count], eax

%ifndef CONFIG32
    call init_64bit_mode
[bits 64]
%else
[bits 32]
%endif
    call main

KERNEL_TASK_SEG_IDX equ 5
KERNEL_TASK_SEG equ 8 * KERNEL_TASK_SEG_IDX
kernel_task_selector: dw KERNEL_TASK_SEG

; Function to load kernel task register.
global load_kernel_tr
load_kernel_tr:
    ltr [kernel_task_selector]
    ret

    jmp $

%ifdef CONFIG32
mem_map_buf_addr: dd 0x0
%else
mem_map_buf_addr: dq 0x0
%endif
mem_map_buf_entry_count: dd 0x0

%include "kernel/asm/kernel64.asm"
%include "kernel/asm/kernel.asm"
