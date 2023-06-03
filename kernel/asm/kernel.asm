[bits 32]

global pm_jump
pm_jump:
    jmp 0x8:pm_jmp_ret
pm_jmp_ret:
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    ret

extern kernel_page_directory
global enable_paging
enable_paging:
    ; point CR3 to page directory
    mov eax, kernel_page_directory
    or eax, 0x3
    mov cr3, eax

    ; set CRO.PG to 1
    mov ebx, cr0	; set left-most bit of CPU special control register.
    or ebx, 0x80000000
    mov cr0, ebx

    ret

USER_TASK_SEG_IDX equ 6
USER_TASK_SEG equ 8 * USER_TASK_SEG_IDX
user_task_selector: dw USER_TASK_SEG

global switch_to_user_task
switch_to_user_task:
    call USER_TASK_SEG: 0x0
    ret

global dummy_branch
dummy_branch:
    mov eax, 0xfadefade
    iret

global __initialize_idt
extern idt_info_ptr

__initialize_idt:
    lidt [idt_info_ptr]
    ret

%include "kernel/asm/print_string_pm.asm"
%include "kernel/asm/interrupts.asm"
