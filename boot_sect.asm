;A simple boot sector program to print hexadecimal value of a given 4-byte value.
[org 0x7c00]

mov dx, 0x1fb6
call print_hex

jmp $

%include "print_string.asm"

times 510 - ($ - $$) db 0

dw 0xaa55

