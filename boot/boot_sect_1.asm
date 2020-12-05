; A boot sector program that enters 32-bit mode and loads the kernel
; into address 0x1000.
[org 0x7c00]
[bits 16]

BOOTLOADER_2_OFFSET equ 0x7e00
BOOTLOADER_2_SIZE_SECTORS equ 0x3

xor ax, ax
mov ds, ax
mov es, ax
mov [BOOT_DRIVE], dl

mov bp, 0x9000
mov ss, bp
mov sp, bp

call load_bootloader_2

; NOTE: 2nd stage bootloader expects boot drive to be stored in dl.
; mov dl, [BOOT_DRIVE]

call BOOTLOADER_2_OFFSET
jmp $

%include "boot/disk_load.asm"
%include "boot/print_string.asm"

[bits 16]
load_bootloader_2:
	mov bx, MSG_2ND_BOOTLOADER
	call print_string
	call print_endl

	mov al,  BOOTLOADER_2_SIZE_SECTORS
	; we don't use ah for anything.
	push ax

	mov bx, BOOTLOADER_2_OFFSET
	push bx

	mov cl, 0x02; 1st stage boot sector in sector 1, 2nd stage in sector 2.
	mov ch, 0x00
	push cx

	mov dl, [BOOT_DRIVE] ; Which drive to use.
	mov dh, 0x00
	push dx

	call disk_load

	pop dx
	pop cx
	pop bx
	pop ax

	ret

BOOT_DRIVE: db 0
MSG_2ND_BOOTLOADER: db "Loading 2nd stage bootloader into memory.", 0

; Pad like before :)
times 510 - ($ - $$) db 0
dw 0xaa55
