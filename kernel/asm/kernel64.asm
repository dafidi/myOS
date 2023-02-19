; Code in this file executes the transition from 32-bit mode to 64-bit mode.
; It needs to be in 32-bit mode. After the `retf` in init_64bit_mode, 64-bit
; mode is enabled and we will be returning to a caller outside this file.
[bits 32]

; 4-level Paging with 2MiB pages.
;
; PDPT = Page Directory Pointer Table
;
; The CR3 register will point to the PML4 which is a table of
; entries pointing to PDPTs.
; Each PDPTE will point to a PD.
; Each PDE will point to a page.
; Each table can have 512 entries and each entry is 64 bits.
; We want to be able to map 8GiB of RAM while ensuring the paging
; structures consume the least amount of memory.
; So, the tables will be structured like this:
;
; Table | Size | # of instances | # of (used) entries per instance |
; PM4L  | 4096 |       1        |                1                 |
; PDPT  | 4096 |       1        |                8                 |
; PD    | 4096 |       8        |                512               |
;
; Note that each PDE points to 2MiB of RAM.
;
; The way this calculates to 8 GiB is:
;
;     1    *    8     *   512    *  2^30 
; (PML4Es) . (PDPTEs) .  (PDEs)  .  (2MiB)
;
; The amount of memory these structures will consume is calculated:
;
;         4096      +       4096        +     8 * 4096     =   40960
; PM4Ls' total size + PDPTs' total size + PDs' total Size
;
NUM_PM4L  equ 1
PM4L_SIZE equ 4096
NUM_PDPT  equ 1
PDPT_SIZE equ 4096
NUM_PD    equ 8
PD_SIZE   equ 4096

setup_4l_paging:

setup_PM4L:
    mov eax, PM4L
    mov ebx, PDPT
    mov ecx, NUM_PDPT
make_PM4LE:
    or ebx, 0x3
    mov [eax], ebx
    add ebx, 4096
    add eax, 8
    dec ecx
    cmp ecx, 0x0
    jne make_PM4LE

setup_PDPT:
    mov eax, PDPT
    mov ebx, PD
    mov ecx, NUM_PD
make_PDPTE:
    or ebx, 0x3
    mov [eax], ebx
    dec ecx
    add ebx, 4096
    add eax, 8
    cmp ecx, 0x0
    jne make_PDPTE

; This is incomplete - we need to setup
; NUM_PD page directories.
    mov edx, NUM_PD
    mov eax, PD
    mov ebx, 0x0
setup_PD:
    mov ecx, 512
setup_PDE:
    or ebx, 0xb3
    mov [eax], ebx
    dec ecx
    add ebx, 0x200000
    add eax, 8
    cmp ecx, 0x0
    jne setup_PDE

    dec edx
    cmp edx, 0x0
    jne setup_PD

ret
; End of setup_4l_paging

load_gdt64:
    cli
    lgdt [gdt64_info]
    ret 
; end of load_gdt64

gdt64_info:
gdt_64_limit:
    dw (gdt64_end - gdt64)
gdt_64_location:
    dq gdt64

align 8
; See IA-32 manual vol. 3 section 3.4.5 for
; desriptions of segment descriptor fields.
global gdt64
gdt64:
    dw 0x0 ; NULL descriptor.
    dw 0x0
    dw 0x0
    dw 0x0
    ; learnt from this stack overflow question:
    ; https://stackoverflow.com/questions/43438363/triple-fault-when-jumping-to-64-bit-longmode
    ; that 64-bit descriptors should set all limit and base to 0.

    ; Kernel Code Segment.
    dw 0x0000	    ; Limit bits 0-15
	dw 0x0000		; Base bits 0-15
	db 0x00		    ; Base bits 16-23
	db 10011011b	; 1st flags, type flags       ; can flip accessed bit, i.e. 10011010b is okay.
	db 10100000b	; 2nd flags, Limit bits 16-19 ; can flip AVL bit, i.e 10110000b is okay.
	db 0x0		    ; Base bits 24-31
    ; or alternatively,
    ; dq 0x00a09b0000000000

    ; Kernel Data Segment.
    dw 0x0000	    ; Limit bits 0-15
	dw 0x0000		; Base bits 0-15
	db 0x00		    ; Base bits 16-23)
	db 10010011b	; 1st flags, type flags         ; can flip accessed bit, i.e. 10010010b is okay.
	db 10100000b	; 2nd flags, Limit (bits 16-19) ; can flip AVL bit, i.e 10110000b is okay.
	db 0x0		    ; Base bits 24-31
    ; or alternatively
    ; dq 0x00a0930000000000

    times 6 dq 0x0
gdt64_end:
; end of gdt

; The description of how to enable 64 bit mode is found in the IA-32 manual,
; Vol. 3, section 9.8.5.
;
IA32_EFER equ 0xc0000080
init_64bit_mode:
    call setup_4l_paging

    ; Disable paging - clear cr0.PG. Should be nop at this point.
    mov eax, cr0
    and eax, 0x7fffffff
    mov cr0, eax

    ; Enable cr4.PAE.
    mov eax, cr4
    or eax, 0x20
    mov cr4, eax

    ; Point cr3 to PM4L.
    mov eax, PM4L
    mov cr3, eax

    ; Enable IA32_EFER.LME.
    ; Find the description of IA32_EFER in
    ; IA32 manual, vol. 4 section 2.2.1 and vol. 3 section 2.2.1.
    mov ecx, IA32_EFER
    rdmsr
    or eax, 0x100
    wrmsr

    ; Enable paging - Set cr0.PG.
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    call load_gdt64

    push 8
    push main
    retf
; End of init_64bit_mode

%include "kernel/asm/interrupts64.asm"

[bits 64]
global __initialize_idt64
extern idt_info_ptr

__initialize_idt64:
    lidt [idt_info_ptr]
    ret

align 4096
; We need to make sure these are 4KiB-aligned.
PM4L:
    times NUM_PM4L * PM4L_SIZE db 0x0
PDPT:
    times NUM_PDPT * PDPT_SIZE db 0x0
PD:
    times NUM_PD * PD_SIZE db 0x0
