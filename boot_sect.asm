; A simple boot sector program to demonstrate using BIOS to load data from disk.
[org 0x7c00]

mov [BOOT_DRIVE], dl

; I think this is only done as a matter of good practice.
; for our small programs, we are probably fine with wherever bp and sp
; are placed. (commented these two lines out and still get correct results.)
;mov bp, 0x8000
;mov sp, bp

mov bx, [where_i_want_to_load_the_data]
mov dh, 2
mov dl, [BOOT_DRIVE]
call disk_load

mov bx, [where_i_want_to_load_the_data]
mov dx, [bx]
call print_hex

mov dx, [bx + 512]
call print_hex

jmp $

%include "print_string.asm"
%include "disk_load.asm"

BOOT_DRIVE: db 0
where_i_want_to_load_the_data: dw 0xa000

times 510 - ($ - $$) db 0

dw 0xaa55

times 256 dw 0xbeef
times 256 dw 0xdeed

