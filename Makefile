
# Remember:
#	$< = first argument for target
#	$^ = all arguments for target
#	$@ = name of target

# Generate list of sources automatically.
C_SOURCES = $(wildcard kernel/*.c drivers/**/*.c fs/*.c)
HEADERS = $(wildcard kernel/*.h drivers/*.h fs/*.h)

C_FLAGS = -Wall -O0 -m32 -fno-pie -fno-stack-protector -ffreestanding
C_FLAGS += -I./

# Generate object file names to build based on *.c filenames.
#OBJ = ${C_SOURCES: .c=.o}
OBJ = $(patsubst %.c, %.o, ${C_SOURCES})

# Default build target.
all: os-image

os_image_dependencies=boot/boot_sect.bin kernel.bin
os-image: $(os_image_dependencies) storage_disk
	cat $(os_image_dependencies) > os-image

storage_disk:
	nasm -f bin -o storage_disk.img storage_disk.asm

kernel.bin: kernel/kernel_entry.o ${OBJ}
	ld -o kernel.bin -m elf_i386 -Ttext 0x1000 $^ --oformat binary

%.o: %.c
	gcc ${C_FLAGS} -c $< -o $@

%.o: %.asm
	nasm $< -f elf -o $@

%.bin: %.asm
	nasm $< -f bin -o $@

#boot_sect.bin: boot_sect.asm
#	nasm $< -f bin -o $@

#kernel_entry.o: kernel_entry.asm
#	nasm $< -f elf -o $@

# The -m32 flag lets the compiler know to compile the kernel for a 32-bit 
# machine. This has a side effect of throwing the error:
# 	"undefined reference to `_GLOBAL_OFFSET_TABLE_'"
# I address this by adding the -fno-pie flag.
#kernel.o: kernel.c
#	gcc -m32 -fno-pie -c $< -o $@

#null.bin: null.asm
#	nasm $< -f bin -o $@

clean:
	rm -rf *.bin *.dis *.o os-image *.map *.img
	rm -rf kernel/*.o boot/*.bin drivers/**/*.o

