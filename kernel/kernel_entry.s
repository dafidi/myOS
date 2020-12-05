/* ;[bits 32] */
	.intel_syntax noprefix

.global initialize_idt
.global mem_map_buf_addr
.global mem_map_buf_entry_count
.global kernel_entry

.extern main
.extern idt_info_ptr

/* ; kernel_entry expects the following information about the */
/* ; BIOS's memory map to be put on the stack: */
/* ;   the address of the buffer holding the memory map (top of stack) */
/* ;   the number of entries in the memory map. */
kernel_entry:
	mov eax, [esp]
	mov [mem_map_buf_addr], eax
	mov eax, [esp+4]
	mov [mem_map_buf_entry_count], eax

	call main
jmp $

mem_map_buf_addr: .word 0x0, 0x0
mem_map_buf_entry_count: .word 0x0, 0x0

.global pm_jump
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

.extern kernel_page_directory
.global setup_and_enable_paging
setup_and_enable_paging:
	/*  ; point CR3 to page directory */
	mov eax, kernel_page_directory
	or eax, 0x3
	mov cr3, eax

	/* ; set CRO.PG to 1 */
	mov ebx, cr0
	/* ; set left-most bit of CPU special control register. */
	or ebx, 0x80000000
	mov cr0, ebx

	/* ; return */
	ret

.equ USER_TASK_SEG_IDX, 7
.equ USER_TASK_SEG, 8 * USER_TASK_SEG_IDX
user_task_selector: .word USER_TASK_SEG

.global switch_task
switch_task:
/*  ;jmp $ */
	call USER_TASK_SEG: 0x0
	ret

.equ KERNEL_TASK_SEG_IDX, 5
.equ KERNEL_TASK_SEG, 8 * KERNEL_TASK_SEG_IDX
kernel_task_selector: .word KERNEL_TASK_SEG

/* Function to load kernel task register. */
.global load_kernel_tr
load_kernel_tr:
	ltr [kernel_task_selector]
	ret

.global dummy_branch
dummy_branch:
	mov eax, 0xfadefade
	call 0x0200119d
	iret

initialize_idt:
	lidt [idt_info_ptr]
	ret

.global enable_interrupts
enable_interrupts:
	sti
	ret

.global disable_interrupts
disable_interrupts:
	cli
	ret

.extern fault_handler
.extern irq_handler

.global isr_common
.global isr0
.global isr1
.global isr2
.global isr3
.global isr4
.global isr5
.global isr6
.global isr7
.global isr8
.global isr9
.global isr10
.global isr11
.global isr12
.global isr13
.global isr14
.global isr15
.global isr16
.global isr17
.global isr18
.global isr19
.global isr20
.global isr21
.global isr22
.global isr23
.global isr24
.global isr25
.global isr26
.global isr27
.global isr28
.global isr29
.global isr30
.global isr31

.global irq0
.global irq1
.global irq2
.global irq3
.global irq4
.global irq5
.global irq6
.global irq7
.global irq8
.global irq9
.global irq10
.global irq11
.global irq12
.global irq13
.global irq14
.global irq15

isr0:
	cli
	# movb [esp], 0x0
	# sub esp, 1
	# movb [esp], 0x0
	# sub esp, 1
	push 0x0
	push 0x0
	jmp isr_common

isr1:
	cli
	# movb [esp], 0x0
	# sub esp, 1
	# movb [esp], 0x1
	# sub esp, 1
	push 0
	push 1
	jmp isr_common

isr2:
	cli
	# movb [esp], 0x0
	# sub esp, 1
	# movb [esp], 0x2
	# sub esp, 1
	push 0
	push 2
	jmp isr_common

isr3:
	cli
	# movb [esp], 0x0
	# sub esp, 1
	# movb [esp], 0x3
	# sub esp, 1
	push 0
	push 3
	jmp isr_common

isr4:
	cli
	# movb [esp], 0x0
	# sub esp, 1
	# movb [esp], 0x4
	# sub esp, 1
	push 0
	push 4
	jmp isr_common

isr5:
	cli
	# movb [esp], 0x0
	# sub esp, 1
	# movb [esp], 0x5
	# sub esp, 1
	push 0
	push 5
	jmp isr_common

isr6:
	cli
	# movb [esp], 0x0
	# sub esp, 1
	# movb [esp], 0x6
	# sub esp, 1
	push 0
	push 6
	jmp isr_common

isr7:
	cli
	# movb [esp], 0x0
	# sub esp, 1
	# movb [esp], 0x7
	# sub esp, 1
	push 0
	push 7
	jmp isr_common

isr8:
	cli
	# movb [esp], 0x8
	# sub esp, 1
	push 8
	jmp isr_common

isr9:
	cli
	# movb [esp], 0x0
	# sub esp, 1
	# movb [esp], 0x9
	# sub esp, 1
	push 0
	push 9
	jmp isr_common

isr10:
	cli
	# movb [esp], 10
	# sub esp, 1
	push 10
	jmp isr_common

isr11:
	cli
	# movb [esp],  11
	# sub esp, 1
	push 11
	jmp isr_common

isr12:
	cli
	# movb [esp],  12
	# sub esp, 1
	push 12
	jmp isr_common

isr13:
	cli
	# movb [esp],  13
	# sub esp, 1
	push 13
	jmp isr_common

isr14:
	cli
	# movb [esp],  14
	# sub esp, 1
	push 14
	jmp isr_common

isr15:
	cli
	# movb [esp],  0
	# sub esp, 1
	# movb [esp],  15
	# sub esp, 1
	push 0
	push 15
	jmp isr_common

isr16:
	cli
	# movb [esp],  0
	# sub esp, 1
	# movb [esp],  16
	# sub esp, 1
	push 0
	push 16
	jmp isr_common

isr17:
	cli
	# movb [esp],  0
	# sub esp, 1
	# movb [esp],  17
	# sub esp, 1
	push 0
	push 17
	jmp isr_common

isr18:
	cli
	# movb [esp],  0
	# sub esp, 1
	# movb [esp],  18
	# sub esp, 1
	push 0
	push 18
	jmp isr_common

isr19:
	cli
	# movb [esp],  0
	# sub esp, 1
	# movb [esp],  19
	# sub esp, 1
	push 0
	push 19
	jmp isr_common

isr20:
	cli
	# movb [esp],  0
	# sub esp, 1
	# movb [esp],  20
	# sub esp, 1
	push 0
	push 20
	jmp isr_common

isr21:
	cli
	# movb [esp],  0
	# sub esp, 1
	# movb [esp],  21
	# sub esp, 1
	push 0
	push 21
	jmp isr_common

isr22:
	cli
	# movb [esp],  0
	# sub esp, 1
	# movb [esp],  22
	# sub esp, 1
	push 0
	push 22
	jmp isr_common

isr23:
	cli
	# movb [esp],  0
	# sub esp, 1
	# movb [esp],  23
	# sub esp, 1
	push 0
	push 23
	jmp isr_common

isr24:
	cli
	# movb [esp],  0
	# sub esp, 1
	# movb [esp],  24
	# sub esp, 1
	push 0
	push 24
	jmp isr_common

isr25:
	cli
	# movb [esp],  0
	# sub esp, 1
	# movb [esp],  25
	# sub esp, 1
	push 0
	push 25
	jmp isr_common

isr26:
	cli
	# movb [esp],  0
	# sub esp, 1
	# movb [esp],  26
	# sub esp, 1
	push 0
	push 26
	jmp isr_common

isr27:
	cli
	# movb [esp],  0
	# sub esp, 1
	# movb [esp],  27
	# sub esp, 1
	push 0
	push 27
	jmp isr_common

isr28:
	cli
	# movb [esp],  0
	# sub esp, 1
	# movb [esp],  28
	# sub esp, 1
	push 0
	push 28
	jmp isr_common

isr29:
	cli
	# movb [esp],  0
	# sub esp, 1
	# movb [esp],  29
	# sub esp, 1
	push 0
	push 29
	jmp isr_common

isr30:
	cli
	# movb [esp],  0
	# sub esp, 1
	# movb [esp],  30
	# sub esp, 1
	push 0
	push 30
	jmp isr_common

isr31:
	cli
	# movb [esp],  0
	# sub esp, 1
	# movb [esp],  31
	# sub esp, 1
	push 0
	push 31
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

irq0:
	cli
	# movb [esp],  0
	# sub esp, 1
	# movb [esp],  32
	# sub esp, 1
	push 0
	push 32
	jmp irq_common

irq1:
	cli
	# movb [esp],  0
	# sub esp, 1
	# movb [esp],  33
	# sub esp, 1
	push 0
	push 33
	jmp irq_common

irq2:
	cli
	# movb [esp],  0
	# sub esp, 1
	# movb [esp],  34
	# sub esp, 1
	push 0
	push 34
	jmp irq_common

irq3:
	cli
	# movb [esp],  0
	# sub esp, 1
	# movb [esp],  35
	# sub esp, 1
	push 0
	push 35
	jmp irq_common

irq4:
	cli
	# movb [esp],  0
	# sub esp, 1
	# movb [esp],  36
	# sub esp, 1
	push 0
	push 36
	jmp irq_common

irq5:
	cli
	# movb [esp],  0
	# sub esp, 1
	# movb [esp],  37
	# sub esp, 1
	push 0
	push 37
	jmp irq_common

irq6:
	cli
	# movb [esp],  0
	# sub esp, 1
	# movb [esp],  38
	# sub esp, 1
	push 0
	push 38
	jmp irq_common

irq7:
	cli
	# movb [esp],  0
	# sub esp, 1
	# movb [esp],  39
	# sub esp, 1
	push 0
	push 39
	jmp irq_common

irq8:
	cli
	# movb [esp],  0
	# sub esp, 1
	# movb [esp],  40
	# sub esp, 1
	push 0
	push 40
	jmp irq_common

irq9:
	cli
	# movb [esp],  0
	# sub esp, 1
	# movb [esp],  41
	# sub esp, 1
	push 0
	push 41
	jmp irq_common

irq10:
	cli
	# movb [esp],  0
	# sub esp, 1
	# movb [esp],  42
	# sub esp, 1
	push 0
	push 42
	jmp irq_common

irq11:
	cli
	# movb [esp],  0
	# sub esp, 1
	# movb [esp],  43
	# sub esp, 1
	push 0
	push 43
	jmp irq_common

irq12:
	cli
	# movb [esp],  0
	# sub esp, 1
	# movb [esp],  44
	# sub esp, 1
	push 0
	push 44
	jmp irq_common

irq13:
	cli
	# movb [esp],  0
	# sub esp, 1
	# movb [esp],  45
	# sub esp, 1
	push 0
	push 45
	jmp irq_common

irq14:
	cli
	# movb [esp],  0
	# sub esp, 1
	# movb [esp],  46
	# sub esp, 1
	push 0
	push 46
	jmp irq_common

irq15:
	cli
	# movb [esp],  0
	# sub esp, 1
	# movb [esp],  47
	# sub esp, 1
	push 0
	push 47
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

.include "kernel/print_string_pm.s"
