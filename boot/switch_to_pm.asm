; A simple program to switch from 16-bit real mode to 32-bit protected mode.
[bits 16]
switch_to_pm:
	cli	; disable interrupts.
	lgdt [gdt_descriptor]

	mov eax, cr0	; set right-most bit of CPU special control register.
	or al, 1
	mov cr0, eax

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

	; BEGIN_PM is a label defined in the second stage bootloader.
	jmp BEGIN_PM

MSG_PROT_MODE: db "Successfully landed in 32-bit Protected Mode [1]", 0

