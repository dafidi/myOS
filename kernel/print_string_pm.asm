; The issue with this function is that we always overwrite the
; the text at the top left of the screen.

[bits 32]
; Define some constants
VIDEO_MEMORY equ 0xb8000
WHITE_ON_BLACK equ 0x0f

; prints a null-terminated string pointed to by ebx
print_string_pm:
	pusha

	; Read cursor position in (put in cx).
	mov ecx, 0x0
	mov ax, 0xE
	mov dx, 0x3D4
	out dx, al
	mov dx, 0x3D5
	in al, dx
	mov ch, al
	mov ax, 0xF
	mov dx, 0x3D4
	out dx, al
	mov dx, 0x3D5
	in al, dx
	mov cl, al
	add ecx, ecx
	mov edx, VIDEO_MEMORY
	add edx, ecx

print_string_pm_loop:
	mov al, [ebx]
	mov ah, WHITE_ON_BLACK

	cmp al, 0
	je print_string_pm_done

	mov [edx], ax

	add ebx, 1
	add edx, 2
	mov ecx, edx
	sub ecx, VIDEO_MEMORY
	shr ecx, 1

	; edx will be used to move cursor, so save it.
	push edx

	; Update cursor position (move forward by 2)
	mov ax, 0xE
	mov dx, 0x3D4
	out dx, al
	mov al, ch
	mov dx, 0x3D5
	out dx, al
	mov ax, 0xF
	mov dx, 0x3D4
	out dx, al
	mov al, cl
	mov dx, 0x3D5
	out dx, al

	pop edx
	jmp print_string_pm_loop

print_string_pm_done:
	popa
	ret

print_hex:
	pusha
	mov ebx, HEX_OUT
	add ebx, 9

to_ascii_loop:
	mov eax, edx
	and eax, 0xf

	cmp eax, 10
	jge to_alpha
	add eax, 48
	jmp end_to_ascii_loop

to_alpha:
	add eax, 87

end_to_ascii_loop:
	mov [ebx], al
	add ebx, -1
	shr edx, 4
	cmp edx, 0
	je exit_print_hex
	jmp to_ascii_loop

exit_print_hex:
	mov ebx, HEX_OUT
	call print_string_pm
	popa
	ret

HEX_OUT:
	db "0x00000000", 0 
; END of print_hex. 
;********************************************************************************
