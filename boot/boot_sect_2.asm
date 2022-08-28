; Second-stage boot loader whose functions are to:
;  - Get the memory map using BIOS.
;  - Load kernel into RAM.
;  - Enter protected mode.
;  - Begin kernel execution.
[org 0x7e00]
[bits 16]

mov [BOOT_DRIVE], dl

; The bootloader(s) occupy RAM from 0x7c00 to 0x83ff.
; Let's load the kernel right after the bootloader.
KERNEL_OFFSET equ 0x8400
KERNEL_SIZE_SECTORS equ 68

mov bx, BOOTLOADER2_START_MSG
call print_string
call print_endl

call get_memory_map
; Store number of memory map entries (returned in dx).
mov [mem_map_buf_entry_count], dx

call load_kernel_1
mov bx, MSG_LOADED_KERNEL
call print_string
call print_endl

; This will jump to BEGIN_PM.
call enter_protected_mode

[bits 32]
BEGIN_PM:
	mov ebx, MSG_SWITCHED_TO_PM
	call print_string_pm

	mov eax, [mem_map_buf_entry_count]
	; The kernel expect the bios memory map count and address to be on 
	; the stack when it starts executing, so push them to the stack.
	push eax
	push mem_map_buff
	
	; 'jmp' may be better here than 'call' because we don't have use for
	; the side effect(s) of 'call'.
	jmp start_kernel 

mem_map_buf_entry_count: dd 0x0
;=======================================================================
; No return from here.
;=======================================================================

%include "boot/disk_load.asm"
%include "boot/gdt.asm"
%include "boot/print_string.asm"
%include "boot/print_hex_32.asm"
%include "boot/print_string_pm.asm"
%include "boot/switch_to_pm.asm"
%include "boot/bios_memory_map.asm"

[bits 16]
get_memory_map:
	mov bx, GET_MEM_MAP_MSG
	call print_string
	call print_endl

	mov di, mem_map_buff
	call BiosGetMemoryMap
  ret

load_kernel_1:
	mov bx, LOADING_KERNEL_MSG
	call print_string
	call print_endl

	mov al,  KERNEL_SIZE_SECTORS
	; we don't use ah for anything.
	push ax

	mov bx, KERNEL_OFFSET
	push bx

	mov cl, 0x05;  1st stage boot sector in sector 1, 2nd stage in sector 2,3,4 kernel in sectors 5, 6, ...
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

enter_protected_mode:
  call switch_to_pm

[bits 32]
start_kernel:
	; 'jmp' may be better here than 'call' because we don't have use for
	; the side effect(s) of 'call'.
	jmp KERNEL_OFFSET

[bits 16]
BOOT_DRIVE: db 0
BOOTLOADER2_START_MSG: db "2nd stage bootloader loaded running.", 0
GET_MEM_MAP_MSG: db "Getting memory map:", 0
LOADING_KERNEL_MSG: db "Loading kernel into RAM.", 0
MSG_LOADED_KERNEL: db "Loaded kernel into RAM.", 0
MSG_SWITCHED_TO_PM: db "Switched to PM!", 0

mem_map_buff: times 168 db 0xff
times 1536 - ($ - $$) db 0

;=======================================================================
; DEBUGGING
; [bits 16]
; load_kernel_2:
; 	mov bx, KERNEL_OFFSET
; 	mov dh, KERNEL_SIZE_SECTORS ; Read this many sectors.
; 	mov dl, [BOOT_DRIVE]

; 	call kernel_disk_load
; 	ret
;=======================================================================
