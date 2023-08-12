C_FLAGS = -Wall -O0 -m32 -nostdlib -fno-pic -fno-pie -fno-stack-protector -fno-hosted -ffreestanding
S_FLAGS = -Wall -O0 -m32 -nostdlib -fno-pic -fno-pie -fno-stack-protector -ffreestanding

GNU_CC=/Users/davidawogbemila/Downloads/Software_Installation/gcc-13.1.0/bin/gcc
OBJCOPY=/usr/local/opt/llvm/bin/llvm-objcopy

s:	clean
	# gcc -o app.s.elf ${S_FLAGS} -T app.ld -Wl,-e,main app.s
	${GNU_CC} ${S_FLAGS} -static -v -g app.s -o app.s.elf -Wl,-e,main \
	-Wl,-pagezero_size,0  -Wl,-segaddr,__TEXT 0x30000000
	objdump -d app.s.elf > app.s.dis
	${OBJCOPY} -O binary --only-section=.text app.s.elf app.bin

c:	clean
	gcc -o app.c.elf ${C_FLAGS} -Ttext=0x4000 -Wl,-emain app.c
	objdump -d app.c.elf > app.c.dis
	objcopy -O binary app.c.elf app.bin

clean:
	rm -rf *.o *.bin
