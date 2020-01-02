
# Remember:
#	$< = first argument for target
#	$^ = all arguments for target
#	$@ = name of target

# Generate list of sources automatically.
C_SOURCES = $(wildcard kernel/*.c kernel/**/*.c drivers/**/*.c fs/*.c)
HEADERS = $(wildcard kernel/*.h kernel/**/*.h drivers/*.h fs/*.h)

C_FLAGS = -Wall -O0 -m32 -fno-pie -fno-stack-protector -ffreestanding -fno-hosted
C_FLAGS += -I./

# Generate object file names to build based on *.c filenames.
#OBJ = ${C_SOURCES: .c=.o}
OBJ = $(patsubst %.c, %.o, ${C_SOURCES})

# Default build target.
all: os-image

boot_sector_deps=boot_sect.bin
kernel_deps=kernel.bin
os_image_deps=$(boot_sector_deps) $(kernel_deps) 

os-image: $(os_image_deps) storage_disk.img
	cat $(os_image_deps) > os-image

boot_sect.bin: boot/boot_sect.bin
	cp boot/boot_sect.bin boot_sect.bin

kernel.bin: kernel/kernel_entry.o ${OBJ}
	ld -o kernel.bin -m elf_i386 -Ttext 0x1000 $^ --oformat binary kernel.ld

storage_disk.img:
	nasm -f bin -o storage_disk.img storage_disk.asm

%.o: %.c
	gcc ${C_FLAGS} -c $< -o $@

%.o: %.asm
	nasm $< -f elf -o $@

%.bin: %.asm
	nasm $< -f bin -o $@

clean:
	rm -rf *.bin *.dis *.o os-image *.map
	rm -rf kernel/*.o kernel/**/*.o boot/*.bin drivers/**/*.o fs/*.o

