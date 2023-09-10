.code64

.globl _asm_enable_interrupts64
_asm_enable_interrupts64:
    sti
    ret

.globl _asm_disable_interrupts64
_asm_disable_interrupts64:
    cli
    ret

.extern _fault_handler64
.extern _irq_handler64

.globl isr64_common
.globl _asm_isr64_0
.globl _asm_isr64_1
.globl _asm_isr64_2
.globl _asm_isr64_3
.globl _asm_isr64_4
.globl _asm_isr64_5
.globl _asm_isr64_6
.globl _asm_isr64_7
.globl _asm_isr64_8
.globl _asm_isr64_9
.globl _asm_isr64_10
.globl _asm_isr64_11
.globl _asm_isr64_12
.globl _asm_isr64_13
.globl _asm_isr64_14
.globl _asm_isr64_15
.globl _asm_isr64_16
.globl _asm_isr64_17
.globl _asm_isr64_18
.globl _asm_isr64_19
.globl _asm_isr64_20
.globl _asm_isr64_21
.globl _asm_isr64_22
.globl _asm_isr64_23
.globl _asm_isr64_24
.globl _asm_isr64_25
.globl _asm_isr64_26
.globl _asm_isr64_27
.globl _asm_isr64_28
.globl _asm_isr64_29
.globl _asm_isr64_30
.globl _asm_isr64_31

.globl _asm_irq64_0
.globl _asm_irq64_1
.globl _asm_irq64_2
.globl _asm_irq64_3
.globl _asm_irq64_4
.globl _asm_irq64_5
.globl _asm_irq64_6
.globl _asm_irq64_7
.globl _asm_irq64_8
.globl _asm_irq64_9
.globl _asm_irq64_10
.globl _asm_irq64_11
.globl _asm_irq64_12
.globl _asm_irq64_13
.globl _asm_irq64_14
.globl _asm_irq64_15

.globl isrs64_begin
.globl isrs64_end
.globl irqs64_begin
.globl irqs64_end

# In 64-bit mode, we can skip pushing dummy error codes (push 0) because
# the processor offers a uniform format for interrupt stack frame regardless
# of whether there is a changein privilege level or not. These are commented
# out. When the code is verified to work, they will be removed.
# See Intel IA-32 Developer's manual Vol. 3 Section 6.14,
# Figure 6-9 for more information.

isrs64_begin:
_asm_isr64_0:
    cli
    push $0
    jmp isr64_common

_asm_isr64_1:
    cli
    push $1
    jmp isr64_common

_asm_isr64_2:
    cli
    push $2
    jmp isr64_common

_asm_isr64_3:
    cli
    push $3
    jmp isr64_common

_asm_isr64_4:
    cli
    push $4
    jmp isr64_common

_asm_isr64_5:
    cli
    push $5
    jmp isr64_common

_asm_isr64_6:
    cli
    push $6
    jmp isr64_common

_asm_isr64_7:
    cli
    push $7
    jmp isr64_common

_asm_isr64_8:
    cli
    push $8
    jmp isr64_common

_asm_isr64_9:
    cli
    push $9
    jmp isr64_common

_asm_isr64_10:
    cli
    push $10
    jmp isr64_common

_asm_isr64_11:
    cli
    push $11
    jmp isr64_common

_asm_isr64_12:
    cli
    push $12
    jmp isr64_common

_asm_isr64_13:
    cli
    push $13
    jmp isr64_common

_asm_isr64_14:
    cli
    push $14
    jmp isr64_common

_asm_isr64_15:
    cli
    push $15
    jmp isr64_common

_asm_isr64_16:
    cli
    push $16
    jmp isr64_common

_asm_isr64_17:
    cli
    push $17
    jmp isr64_common

_asm_isr64_18:
    cli
    push $18
    jmp isr64_common

_asm_isr64_19:
    cli
    push $19
    jmp isr64_common

_asm_isr64_20:
    cli
    push $20
    jmp isr64_common

_asm_isr64_21:
    cli
    push $21
    jmp isr64_common

_asm_isr64_22:
    cli
    push $22
    jmp isr64_common

_asm_isr64_23:
    cli
    push $23
    jmp isr64_common

_asm_isr64_24:
    cli
    push $24
    jmp isr64_common

_asm_isr64_25:
    cli
    push $25
    jmp isr64_common

_asm_isr64_26:
    cli
    push $26
    jmp isr64_common

_asm_isr64_27:
    cli
    push $27
    jmp isr64_common

_asm_isr64_28:
    cli
    push $28
    jmp isr64_common

_asm_isr64_29:
    cli
    push $29
    jmp isr64_common

_asm_isr64_30:
    cli
    push $30
    jmp isr64_common

_asm_isr64_31:
    cli
    push $31
    jmp isr64_common

isr64_common:
    push %rax
    push %rcx
    push %rdx
    push %rbx
    push %rsp
    push %rbp
    push %rsi
    push %rdi
    mov %rsp, %rax
    mov %rax, %rdi
    callq _fault_handler64
    pop %rdi
    pop %rsi
    pop %rbp
    pop %rsp
    pop %rbx
    pop %rdx
    pop %rcx
    pop %rax
    add $16, %rsp
    iretq
isrs64_end:

# Error codes are not pushed for additional interrupt service handlers. Let's "push 0"
# just for consistency.
irqs64_begin:
_asm_irq64_0:
    cli
    push $0
    push $32
    jmp irq64_common

_asm_irq64_1:
    cli
    push $0
    push $33
    jmp irq64_common

_asm_irq64_2:
    cli
    push $0
    push $34
    jmp irq64_common

_asm_irq64_3:
    cli
    push $0
    push $35
    jmp irq64_common

_asm_irq64_4:
    cli
    push $0
    push $36
    jmp irq64_common

_asm_irq64_5:
    cli
    push $0
    push $37
    jmp irq64_common

_asm_irq64_6:
    cli
    push $0
    push $38
    jmp irq64_common

_asm_irq64_7:
    cli
    push $0
    push $39
    jmp irq64_common

_asm_irq64_8:
    cli
    push $0
    push $40
    jmp irq64_common

_asm_irq64_9:
    cli
    push $0
    push $41
    jmp irq64_common

_asm_irq64_10:
    cli
    push $0
    push $42
    jmp irq64_common

_asm_irq64_11:
    cli
    push $0
    push $43
    jmp irq64_common

_asm_irq64_12:
    cli
    push $0
    push $44
    jmp irq64_common

_asm_irq64_13:
    cli
    push $0
    push $45
    jmp irq64_common

_asm_irq64_14:
    cli
    push $0
    push $46
    jmp irq64_common

_asm_irq64_15:
    cli
    push $0
    push $47
    jmp irq64_common

irq64_common:
    push %rax
    push %rcx
    push %rdx
    push %rbx
    push %rsp
    push %rbp
    push %rsi
    push %rdi
    mov %rsp, %rax
    mov %rax, %rdi
    callq _irq_handler64
    pop %rdi
    pop %rsi
    pop %rbp
    pop %rsp
    pop %rbx
    pop %rdx
    pop %rcx
    pop %rax
    add $0x10, %rsp
    iretq
_asm_irqs64_end:
