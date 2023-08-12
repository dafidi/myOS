.code32

.globl main
main:
    lea exit(,1), %eax
    jmp $0x1b, $exit
exit:
    iret

.data
.fill 10, 1, 0x0
