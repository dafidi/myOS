[bits 32]

global initialize_idt
global mem_map_buf_addr
global mem_map_buf_entry_count
global kernel_entry

extern main
extern idt_info_ptr

; kernel_entry expects the following information about the
; BIOS's memory map to be put on the stack:
;   the address of the buffer holding the memory map (top of stack)
;   the number of entries in the memory map.
kernel_entry:
  	mov eax, [esp]
  	mov [mem_map_buf_addr], eax
  	mov eax, [esp+4]
  	mov [mem_map_buf_entry_count], eax

  	call main
jmp $

mem_map_buf_addr: dd 0x0
mem_map_buf_entry_count: dd 0x0

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

KERNEL_TASK_SEG_IDX equ 5
KERNEL_TASK_SEG equ 8 * KERNEL_TASK_SEG_IDX
kernel_task_selector: dw KERNEL_TASK_SEG

; Function to load kernel task register.
global load_kernel_tr
load_kernel_tr:
	ltr [kernel_task_selector]
	ret

global dummy_branch
dummy_branch:
	mov eax, 0xfadefade
	iret

initialize_idt:
	lidt [idt_info_ptr]
	ret

global enable_interrupts
enable_interrupts:
	sti
	ret

global disable_interrupts
disable_interrupts:
	cli
	ret

extern fault_handler
extern irq_handler

global isr_common
global isr0
global isr1
global isr2
global isr3
global isr4
global isr5
global isr6
global isr7
global isr8
global isr9
global isr10
global isr11
global isr12
global isr13
global isr14
global isr15
global isr16
global isr17
global isr18
global isr19
global isr20
global isr21
global isr22
global isr23
global isr24
global isr25
global isr26
global isr27
global isr28
global isr29
global isr30
global isr31

global irq0
global irq1
global irq2
global irq3
global irq4
global irq5
global irq6
global irq7
global irq8
global irq9
global irq10
global irq11
global irq12
global irq13
global irq14
global irq15

global isrs_begin
global isrs_end
global irqs_begin
global irqs_end

isrs_begin:
isr0:
	cli
	push byte 0
	push byte 0
	jmp isr_common

isr1:
	cli
	push byte 0
	push byte 1
	jmp isr_common

isr2:
	cli
	push byte 0
	push byte 2
	jmp isr_common

isr3:
  	cli
  	push byte 0
  	push byte 3
  	jmp isr_common

isr4:
	cli
	push byte 0
	push byte 4
	jmp isr_common

isr5:
	cli
	push byte 0
	push byte 5
	jmp isr_common

isr6:
	cli
	push byte 0
	push byte 6
	jmp isr_common

isr7:
	cli
	push byte 0
	push byte 7
	jmp isr_common

isr8:
	cli
	push byte 8
	jmp isr_common

isr9:
	cli
	push byte 0
	push byte 9
	jmp isr_common

isr10:
	cli
	push byte 10
	jmp isr_common

isr11:
	cli
	push byte 11
	jmp isr_common

isr12:
	cli
	push byte 12
	jmp isr_common

isr13:
	cli
	push byte 13
	jmp isr_common

isr14:
	cli
	push byte 14
	jmp isr_common

isr15:
	cli
	push byte 0
	push byte 15
	jmp isr_common

isr16:
	cli
	push byte 0
	push byte 16
	jmp isr_common

isr17:
	cli
	push byte 0
	push byte 17
	jmp isr_common

isr18:
	cli
	push byte 0
	push byte 18
	jmp isr_common

isr19:
	cli
	push byte 0
	push byte 19
	jmp isr_common

isr20:
	cli
	push byte 0
	push byte 20
	jmp isr_common

isr21:
	cli
	push byte 0
	push byte 21
	jmp isr_common

isr22:
	cli
	push byte 0
	push byte 22
	jmp isr_common

isr23:
	cli
	push byte 0
	push byte 23
	jmp isr_common

isr24:
	cli
	push byte 0
	push byte 24
	jmp isr_common

isr25:
	cli
	push byte 0
	push byte 25
	jmp isr_common

isr26:
	cli
	push byte 0
	push byte 26
	jmp isr_common

isr27:
	cli
	push byte 0
	push byte 27
	jmp isr_common

isr28:
	cli
	push byte 0
	push byte 28
	jmp isr_common

isr29:
	cli
	push byte 0
	push byte 29
	jmp isr_common

isr30:
	cli
	push byte 0
	push byte 30
	jmp isr_common

isr31:
	cli
	push byte 0
	push byte 31
	jmp isr_common

isr_common:
	pusha
	push ds
	push es
	push fs
	push gs
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov eax, esp
	push eax
	mov eax, fault_handler
	call eax
	pop eax
	pop gs
	pop fs
	pop es
	pop ds
	popa
	add esp, 8
	iret
isrs_end:

irqs_begin:
irq0:
	cli
	push byte 0
	push byte 32
	jmp irq_common

irq1:
	cli
	push byte 0
	push byte 33
	jmp irq_common

irq2:
	cli
	push byte 0
	push byte 34
	jmp irq_common

irq3:
	cli
	push byte 0
	push byte 35
	jmp irq_common

irq4:
	cli
	push byte 0
	push byte 36
	jmp irq_common

irq5:
	cli
	push byte 0
	push byte 37
	jmp irq_common

irq6:
	cli
	push byte 0
	push byte 38
	jmp irq_common

irq7:
	cli
	push byte 0
	push byte 39
	jmp irq_common

irq8:
	cli
	push byte 0
	push byte 40
	jmp irq_common

irq9:
	cli
	push byte 0
	push byte 41
	jmp irq_common

irq10:
	cli
	push byte 0
	push byte 42
	jmp irq_common

irq11:
	cli
	push byte 0
	push byte 43
	jmp irq_common

irq12:
	cli
	push byte 0
	push byte 44
	jmp irq_common

irq13:
	cli
	push byte 0
	push byte 45
	jmp irq_common

irq14:
	cli
	push byte 0
	push byte 46
	jmp irq_common

irq15:
	cli
	push byte 0
	push byte 47
	jmp irq_common

irq_common:
	pusha
	push ds
	push es
	push fs
	push gs
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov eax, esp
	push eax
	mov eax, irq_handler
	call eax
	pop eax
	pop gs
	pop fs
	pop es
	pop ds
	popa
	add esp, 8
	iret
irqs_end:

%include "kernel/print_string_pm.asm"
