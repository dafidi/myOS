; A simple boot sector program to use BIOS to load data from disk.
; Parameters:

; EXTERNAL
; dl => indicate (index) desired drive.
; bx => indicate address in memory (offsetted from es) where BIOS will load the data to.

; INTERNAL
; ah => indicate BIOS read sector function (interrupt).
; al => indicate number of sectors to be read
; ch => select cylinder (or track; i think mean the same thing)
; dh => select the side of the desired platter => {
; 		i.e 0 => first side of first platter
;		    1 => second side of first platter
;		    2 => first side of second platter ... and so on.
;	}
; 	 Basically, given n, floor(n/2) indicates platter index, 
;		             n%2 indicates "top" (0) or "bottom" face (1).
; cl => indicates index of first sector from which to start reading.

; Also disk read interrupt is 0x13.

disk_load:
	push dx
	
	mov ah, 0x02
	mov al, dh
	mov ch, 0x00
	mov dh, 0x00
	mov cl, 0x02

	int 0x13
	jc call_disk_error

	pop dx

	cmp dh, al
	jne call_disk_error_mismatch

finish_disk_load:
	ret

call_disk_error:
	call display_disk_error
	ret

call_disk_error_mismatch:
	call display_disk_error_mismatch
	ret
;;;;;;;;;;;;;;;;
;;Helper stuff;;
;;;;;;;;;;;;;;;;
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

DISK_ERROR_MSG_MISMATCH:
	db "Error reading from disk MISMATCH!", 0

DISK_ERROR_MSG:
	db "Error reading from disk NO MISMATCH!", 0

DISK_LOAD_DONE_MSG:
	db "Done loading from disk. (No problems)", 0
