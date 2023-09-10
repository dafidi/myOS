.code32

.equ NUM_PM4L,   1
.equ PM4L_SIZE,  4096
.equ NUM_PDPT,   1
.equ PDPT_SIZE,  4096
.equ NUM_PD,     8
.equ PD_SIZE,    4096

setup_4l_paging:
setup_PM4L:
    mov $PM4L, %eax
    mov $PDPT, %ebx
    mov $1, %ecx
make_PM4LE:
    or $0x3, %ebx
    mov %ebx, (%eax)
    add $0x1000, %ebx
    add $0x8, %ebx
    dec %ecx
    cmp $0x0, %ecx
    jne make_PM4LE

setup_PDPT:
    mov $PDPT, %eax
    mov $PD, %ebx
    mov $NUM_PD, %ecx
make_PDPTE:
    or $0x3, %ebx
    mov %ebx, (%eax)
    dec %ecx
    add $0x1000, %ebx
    add $8, %eax
    cmp $0x0, %ecx
    jne make_PDPTE

    mov $NUM_PD, %edx
    mov $PD, %eax
    mov $0x0, %ebx
setup_PD:
    mov $0x200, %ecx
setup_PDE:
    or $0xb3, %ebx
    mov %ebx, (%eax)
    dec %ecx
    add $0x200000, %ebx
    add $0x8, %eax
    cmp $0x0, %ecx
    jne setup_PDE

    dec %edx
    cmp $0x0, %edx
    jne setup_PD

    ret
    # End of setup_4l_paging

load_gdt64:
    cli
    lgdt gdt64_info(,1)
    ret 
# end of load_gdt64
#
gdt64_info:
gdt_64_limit:
#    # Previously had ".short gdt64_end - gdt64" here but doesn't work because
#    # that expression is 32 bits and short is only 16 bits. But the format
#    # expected by x86 processors is 16 bits. So let'st just hardcode the value
#    # here. If changing the size of gdt64, must change this.
    .short 0x0038
gdt_64_location:
    .quad _gdt64

.balign 8
## See IA-32 manual vol. 3 section 3.4.5 for
## desriptions of segment descriptor fields.
.globl _gdt64
_gdt64:
    .word 0x0 # NULL descriptor.
    .word 0x0
    .word 0x0
    .word 0x0
    # learnt from this stack overflow question:
    # https://stackoverflow.com/questions/43438363/triple-fault-when-jumping-to-64-bit-longmode
    # that 64-bit descriptors should set all limit and base to 0.

    # Kernel Code Segment.
    .word 0x0000	    # Limit bits 0-15
	.word 0x0000		# Base bits 0-15
	.byte 0x00		    # Base bits 16-23
	.byte 0b10011011	    # 1st flags, type flags       ; can flip accessed bit, i.e. 10011010b is okay.
	.byte 0b10100000	    # 2nd flags, Limit bits 16-19 ; can flip AVL bit, i.e 10110000b is okay.
	.byte 0x0		    # Base bits 24-31
    # or alternatively,
    # .quad 0x00a09b0000000000

    # Kernel Data Segment.
    .word 0x0000	    # Limit bits 0-15
	.word 0x0000		# Base bits 0-15
	.byte 0x00		    # Base bits 16-23
	.byte 0b10010011	    # 1st flags, type flags         # can flip accessed bit, i.e. 10010010b is okay.
	.byte 0b10100000	    # 2nd flags, Limit (bits 16-19) # can flip AVL bit, i.e 10110000b is okay.
	.byte 0x0		    # Base bits 24-31
    # or alternatively
    # .quad $0x00a0930000000000

    .fill 48, 1, 0x0
gdt64_end:
# end of gdt

# The description of how to enable 64 bit mode is found in the IA-32 manual,
# Vol. 3, section 9.8.5.
#
.set IA32_EFER, 0xc0000080
init_64bit_mode:
    call setup_4l_paging

    # Disable paging - clear cr0.PG. Should be nop at this point.
    mov %cr0, %eax
    and $0x7fffffff, %eax
    mov %eax, %cr0

    # Enable cr4.PAE.
    mov %cr4, %eax
    or $0x20, %eax
    mov %eax, %cr4

    # Point cr3 to PM4L.
    mov $PM4L, %eax
    mov %eax, %cr3

    # Enable IA32_EFER.LME.
    # Find the description of IA32_EFER in
    # IA32 manual, vol. 4 section 2.2.1 and vol. 3 section 2.2.1.
    mov $IA32_EFER, %ecx
    rdmsr
    or $0x100, %eax
    wrmsr

    # Enable paging - Set cr0.PG.
    mov %cr0, %eax
    or $0x80000000, %eax
    mov %eax, %cr0

    call load_gdt64

    push $8
    leal _main(,1), %eax
    push %eax
    # Found out from https://stackoverflow.com/a/68379295/5741726 to use lret
    # instead of retf.
    lret
# End of init_64bit_mode

.include "kernel/asm/interrupts64.s"

.code64
.globl _asm_initialize_idt64
.extern _idt_info_ptr

_asm_initialize_idt64:
    lidt _idt_info_ptr(%rip)
    ret

# Per https://stackoverflow.com/a/52368791/5741726, .align 12 this means align
# to 2^12, but it seems in some cases it is intepreted as .align 12. So just use
# the explicit .balign 0x1000.
#.data
.balign 0x1000
# We need to make sure these are 4KiB-aligned.
PM4L:
    .fill NUM_PM4L * PM4L_SIZE, 1, 0x0
PDPT:
    .fill NUM_PDPT * PDPT_SIZE, 1, 0x0
PD:
    .fill NUM_PD * PD_SIZE, 1, 0x0
