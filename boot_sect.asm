; A boot sector program that enters 32-bit mode.
[org 0x7c00]

KERNEL_OFFSET equ 0x1000

mov [BOOT_DRIVE], dl

mov bp, 0x9000
mov sp, bp

mov bx, MSG_REAL_MODE
call print_string

call print_endl

call load_kernel

call switch_to_pm

jmp $

%include "disk_load.asm"
%include "gdt.asm"
%include "print_string.asm"
%include "switch_to_pm.asm"
%include "print_string_pm.asm"

[bits 16]
load_kernel:
	mov bx, MSG_LOAD_KERNEL
	call print_string

	mov bx, KERNEL_OFFSET
	mov dh, 15
	mov dl, [BOOT_DRIVE]
	call disk_load

	ret

[bits 32]
BEGIN_PM:

mov ebx, MSG_PROT_MODE
call print_string_pm

call KERNEL_OFFSET

jmp $

BOOT_DRIVE: db 0
MSG_REAL_MODE: db "Started in 16-bit real mode", 0
MSG_PROT_MODE: db "Successfully landed in 32-bit Protected Mode", 0
MSG_LOAD_KERNEL: db "Loading kernel into memory.", 0

; Pad like before :)
times 510 - ($ - $$) db 0
dw 0xaa55

