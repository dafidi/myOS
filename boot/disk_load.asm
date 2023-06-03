;=======================================================================
; A simple boot sector program to use BIOS to load data from disk.
;=======================================================================
;=======================================================================
; Parameter format:
;
;		  							  															 									byte 1  | byte 2
; bottom of the stack ==>				number of sectors | unused
;	    																						memory location  | memory location
;	    																										 sector_offset | cyclinder/track
;	top of stack =========>	 						desired_drive  | side of platter
;
; Setting up registers for disk read via BIOS function:
; 	dl => indicate (index) desired drive.
; 	dh => select the side of the desired platter => {
; 			i.e 0 => first side of first platter
;			   		  1 => second side of first platter
;			    		2 => first side of second platter ... and so on.
;		}
; 	cl => indicates index of first sector from which to start reading.
; 	ch => select cylinder (or track; i think they mean the same thing)
; 	bx => indicate address in memory (offsetted from es) where BIOS will load the data to.
; 	al => indicate number of sectors to be read
; 	ah => indicate BIOS read sector function (interrupt).
;
; Also disk read interrupt is 0x13.
;=======================================================================

[bits 16]

MAX_READ_SECTORS equ 128

disk_load:
	mov bp, sp
	mov bx, DISK_MSG
	call print_string
	call print_endl

						;              byte 1  | byte 2
	mov dx, [bp + 2]    ;    desired drive     | side of platter
	mov cx, [bp + 4]    ;    sector offset     | cyclinder/track
	mov bx, [bp + 6]    ;    memory location   | memory location
	mov ax, [bp + 8]    ;    number of sectors | unused

read_max_sectors:
	mov ah, al
	cmp al, 128
	jg int_0x13
	mov al, 128

int_0x13:
	sub ah, al
	push dx ;  Let's save the desired drive info as we want to use dx for other stuff later.
	push ax ; let's remember the number of sectors we wanted to read 
			; (stored in al).

	mov ah, 0x02
	int 0x13
	jc call_disk_error

	pop dx 	; let's remember the number of sectors we wanted to read.
			; (read into dl).

	cmp dl, al
	jne call_disk_error_mismatch

	test dh, 0xff
	jz finish_disk_load

	; Update the sector offset.
	add cl, dl

	; Update the memory location.
	; First save the number of sectors we still have to read.
	mov al, dh
	; Then, multiply the number of sectors just read by the sector size
	; and add the product to the destination location (in bx)
	push ax
	mov dh, 0
	mov ax, dx
	mov dx, 512
	mul dx ; mul must use ax, so we have to save ax, do the  mul and then retrieve it.
	add bx, ax
	pop ax

	pop dx ; Recover the desired drive info.
	jmp read_max_sectors

finish_disk_load:
	pop dx
	mov sp, bp
	ret

call_disk_error:
	call display_disk_error
	jmp finish_disk_load

call_disk_error_mismatch:
	call display_disk_error_mismatch
	jmp finish_disk_load

;=======================================================================
; Helper stuff
;=======================================================================
display_disk_error:
	push bx
	mov bx, DISK_ERROR_MSG
	call print_string
	pop bx
	ret

display_disk_error_mismatch:
	push bx
	mov bx, DISK_ERROR_MSG_MISMATCH
	call print_string
	pop bx
	ret

DISK_MSG:
	db "DISK_LOADER IN ACTION!", 0

DISK_ERROR_MSG_MISMATCH:
	db "Error reading from disk MISMATCH!", 0

DISK_ERROR_MSG:
	db "Error reading from disk NO MISMATCH!", 0

DISK_LOAD_DONE_MSG:
	db "Done loading from disk. No problems!", 0

;=======================================================================
; FOR DEBUGGING
; [bits 16]
; kernel_disk_load:
; 	mov bp, sp

; push dx
	
; 	mov ah, 0x02
; 	mov al, dh
; 	mov ch, 0x00
; 	mov dh, 0x00
; 	mov cl, 0x04; Boot sector(s) in sectors 1, 2 and 3, kernel in sectors 4, 5, ...

; 	int 0x13
; 	jc call_disk_error

; 	pop dx

; 	cmp dh, al
; 	jne call_disk_error_mismatch

; finish_kernel_disk_load:
; 	jmp finish_disk_load
;=======================================================================
