C_FLAGS = -Wall -O0 -m32 -nostdlib -fno-pic -fno-pie -fno-stack-protector -fno-hosted -ffreestanding
S_FLAGS = -Wall -O0 -m32 -nostdlib -fno-pic -fno-pie -fno-stack-protector -fno-hosted -ffreestanding

s:	clean
	gcc -o app.s.elf ${S_FLAGS} -Ttext=0x4000 -Wl,-emain app.s
	objdump -d app.s.elf > app.s.dis
	objcopy -O binary --only-section=.text app.s.elf app.bin

c:	clean
	gcc -o app.c.elf ${C_FLAGS} -Ttext=0x4000 -Wl,-emain app.c
	objdump -d app.c.elf > app.c.dis
	objcopy -O binary app.c.elf app.bin

clean:
	rm -rf *.o *.bin
