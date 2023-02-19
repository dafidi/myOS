; Second-stage boot loader whose functions are to:
;  - Get the memory map using BIOS.
;  - Load kernel into RAM.
;  - Enter protected mode.
;  - Begin kernel execution.
[org 0x7e00]
[bits 16]

mov [BOOT_DRIVE], dl

; The bootloader(s) occupy RAM from 0x7c00 to 0x85ff.
; Let's load the kernel after the bootloader. The
; linking loader (ld) seems to round up whatever is
; specified in kernel.ld to be 4KiB-aligned so we might
; as well use such an alignment directly.
KERNEL_OFFSET equ 0x9000
KERNEL_SIZE_SECTORS equ 200

mov bx, BOOTLOADER2_START_MSG
call print_string
call print_endl

call get_memory_map
; Store number of memory map entries (returned in dx).
mov [mem_map_buf_entry_count], dx


; This will jump to BEGIN_PM (see switch_to_pm.asm).
call enter_protected_mode

[bits 32]
BEGIN_PM:
	mov ebx, MSG_SWITCHED_TO_PM
	call print_string_pm

	call load_kernel_pm

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

[bits 32]
poll_status_register:
	pusha

	mov edx, 0x1f7
	in al, dx
	and al, 0xc0
	cmp al, 0x40
	jne poll_status_register

	popa
	ret

load_kernel_pm:
	cli ; disable interrupts

	mov edx, 0x1f6
	mov eax, 0xe0
	; TODO: might need to set last 4 bits of ebx, drive_select_port.
	out dx, al ; set drive select port

	mov ecx, 14
	mov edx, 0x1f7
status_trust_loop:
	in al, dx
	dec ecx
	test ecx, 0xffffffff
	jnz status_trust_loop

	;call poll_status_register

	mov edx, 0x1f1
	mov eax, 0x0
	out dx, al ; set error register/port.

	mov edx, 0x1f2
	mov eax, KERNEL_SIZE_SECTORS
	out dx, al ; set count port.

	mov edx, 0x1f3
	mov eax, 0x5  ; kernel should start from sector 5(0-index)/6(1-index)
	out dx, al ; lba low port
	shr eax, 8
	mov edx, 0x1f4
	out dx, al ; lba mid port
	shr eax, 8
	mov edx, 0x1f5
	out dx, al ; lba high port

	;call poll_status_register

	mov edx, 0x1f7
	mov eax, 0xc4
	out dx, al ; set command port

	mov eax, KERNEL_SIZE_SECTORS
	shl eax, 9
	mov edx, 0x1f0 ; set data port
	mov ebx, KERNEL_OFFSET

DRQ equ 8192
insw_loop:
	mov ecx, eax    ; Issue an insw for only
	cmp eax, DRQ    ; DRQ bytes at a time.
	jle do_insw
	mov ecx, DRQ

do_insw:
	push ecx
	shr ecx, 1

	; poll_status_register
	; This stub does pretty  much the same thing
	; as poll_status_register, but for some reason
	; `call poll_status_register` doesn't work
	; well - we seem to read data in a few bytes
    ; off from the target KERNEL_OFFSET, as if
	; the device is late/slow and only transfers
	; the data _after_ $edi has already been
	; incremented. what a pain in the culo this
	; has been to debug.
	push eax
	push edx
	mov edx, 0x1f7
status_wait:
	in al, dx
	and al, 0xc0
	cmp al, 0x40
	jne status_wait
	pop edx
	pop eax

	mov edi, ebx
	cld
	rep insw

	pop ecx
	sub eax, ecx
	add ebx, ecx

	cmp eax, 0x0
	jne insw_loop

	; sti ; ideally we should enable interrupts
	; here but we haven't set up the disk properly,
	; so let's not wreak havoc on the universe.
	ret

[bits 16]
get_memory_map:
	mov bx, GET_MEM_MAP_MSG
	call print_string
	call print_endl

	mov di, mem_map_buff
	call BiosGetMemoryMap
	ret

enter_protected_mode:
  call switch_to_pm

[bits 32]
start_kernel:
	mov eax, [mem_map_buf_entry_count]
	; The kernel expect the bios memory map count and address to be on 
	; the stack when it starts executing, so push them to the stack.
	push eax
	push mem_map_buff
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
times 2048 - ($ - $$) db 0

