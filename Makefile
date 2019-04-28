
all: os-image

os-image: boot_sect.bin kernel.bin null.bin
	cat $^ > os-image

kernel.bin: kernel.o
	ld -o kernel.bin -Ttext 0x1000 $^ --oformat binary

kernel.o: kernel.c
	gcc -ffreestanding -c $< -o $@

boot_sect.bin: boot_sect.asm
	nasm $< -f bin -o $@

null.bin: null.asm
	nasm $< -f bin -o $@

clean:
	rm -rf *.bin *.dis *.o os-image *.map

