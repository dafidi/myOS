[bits 16]
struc memory_map_entry
	.base         resq 1
	.length       resq 1
	.type         resq 1
	.acpi_null    resq 1
endstruc

;---------------------------------------------
;	Get memory map from bios
;	/in es:di->destination buffer for entries
;	/ret bp=entry count
;---------------------------------------------

BiosGetMemoryMap:
	xor	ebx, ebx
	xor	bp, bp			; number of entries stored here
	mov	edx, 'PAMS'		; 'SMAP'
	mov	eax, 0xe820
	mov	ecx, 24			; memory map entry struct is 24 bytes
	int	0x15			; get first entry
	jc	.error
	cmp	eax, 'PAMS'		; bios returns SMAP in eax
	jne	.error
	test ebx, ebx		; if ebx=0 then list is one entry long; bail out
	je	.error
	jmp	.start
.next_entry:
	mov	edx, 'PAMS'		; some bios's trash this register
	mov	ecx, 24			; entry is 24 bytes
	mov	eax, 0xe820
	int	0x15			; get next entry
.start:
	jcxz	.skip_entry		; if actual returned bytes is 0, skip entry
.notext:
	mov	ecx, [es:di + memory_map_entry.length]	; get length (low dword)
	test	ecx, ecx		; if length is 0 skip it
	jne	short .good_entry
	mov	ecx, [es:di + memory_map_entry.length + 4]; get length (upper dword)
	jecxz	.skip_entry		; if length is 0 skip it
.good_entry:
	inc	bp			; increment entry count
	add	di, 24			; point di to next entry in buffer
.skip_entry:
	cmp	ebx, 0			; if ebx return is 0, list is done
	jne	.next_entry		; get next entry
	jmp	.done
.error:
	mov bx, tag_error
	call print_string
	call print_endl
	stc
.done:
	push bp
	call show_result
	pop dx                    ; Return count in dx.
	ret

show_result:
	mov di, mem_map_buff

	mov bx, tag_base
	call print_string

	mov bx, tag_length
	call print_string

	;mov edx, [test+4]
	;call print_hex_32
	;mov edx, [test]
	;call print_hex_32
	call print_endl

	jmp show_result_loop

;test:
	;dd 0xa0b0c0d0
	;dd 0xe0f01020

show_result_loop:
	cmp bp, 0
	je show_result_end

	dec bp
	mov edx, [es:di+4]
	call print_hex_32

	mov edx, [es:di]
	call print_hex_32

	mov edx, [es:di + 12]
	call print_hex_32

	mov edx, [es:di + 8]
	call print_hex_32
	call print_endl

	; mov bx, tag_type
	; call print_string

    ; mov edx, [es:di + 16]
    ; call print_hex_32
    ; call print_endl

    ; mov bx, tag_acpi_null
    ; call print_string

    ; mov edx, [es:di + 20]
    ; call print_hex_32
    ; call print_endl

    add di, 24
    jmp show_result_loop

show_result_end:
	ret

print_good_entry:
	push bx
	add bp, 48
	mov dx, bp
	mov cx, [MSG_good_entry]
	mov dh, ch
	mov  [MSG_good_entry], dx
	sub bp, 48
	mov bx, MSG_good_entry
	call print_string
	call print_endl
	pop bx
	ret

tag_getting_memory_map: db "getting memory map", 0
tag_base: 				db "base:      ", 0
tag_length: 			db "length:    ", 0
tag_type: 				db "type:      ", 0
tag_acpi_null: 			db "acpi_null: ", 0
tag_error: 				db "error!", 0

MSG_good_entry: 		db "-: good entry", 0
