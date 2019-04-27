; A boot sector program that enters 32-bit mode.
[org 0x7c00]

mov bp, 0x9000
mov sp, bp

mov bx, MSG_REAL_MODE
call print_string

call print_endl

call switch_to_pm

jmp $

%include "gdt.asm"
%include "print_string.asm"
%include "switch_to_pm.asm"
%include "print_string_pm.asm"


[bits 32]
BEGIN_PM:

mov ebx, MSG_PROT_MODE
call print_string_pm

jmp $

MSG_REAL_MODE: db "Started in 16-bit real mode", 0
MSG_PROT_MODE: db "Successfully landed in 32-bit Protected Mode", 0

; Pad like before :)
times 510 - ($ - $$) db 0
dw 0xaa55

