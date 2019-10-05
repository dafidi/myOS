; A simple program to switch from 16-bit real mode to 32-bit protected mode.

switch_to_pm:
	cli	; disable interrupts.
	lgdt [gdt_descriptor]

	mov eax, cr0	; set right-most bit of CPU special control register.
	or al, 1
	mov cr0, eax

	; for some reason, this print statement does not work - perhaps the
	; "far-jump" is needed.
	;mov ebx, debug_msg
	;call print_string_pm

	jmp CODE_SEG:init_pm	; cause the CPU to flush real-mode 16 bit 
				; instructions in pipeline.

[bits 32]
init_pm:
	mov ax, DATA_SEG ;DATA_SEG is defined in gdt.asm
	mov ds, ax
	mov ss, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	mov ebp, 0x90000
	mov esp, ebp

	mov ebx, MSG_PROT_MODE
	call print_string_pm

	;ret
	call BEGIN_PM

MSG_PROT_MODE: db "Successfully landed in 32-bit Protected Mode", 0


