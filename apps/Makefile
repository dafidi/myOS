C_FLAGS = -Wall -O0 -m32 -fno-pie -fno-stack-protector -ffreestanding -fno-hosted

default: app.o
	ld -o app.bin -m elf_i386 --oformat binary -T app.ld\
	 /home/david/dev/myOS/drivers/screen/screen.o /home/david/dev/myOS/kernel/low_level.o app.o

app.o:
	gcc ${C_FLAGS} -o app.o -c -I/home/david/dev/myOS/drivers app.c
