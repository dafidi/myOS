#!/bin/bash
#
# File: build.sh
# 
# This ought to be run in a Linux shell.

# Stop on any error.
set -e

set -x

mode=$1
platform=$2

kernel_target=kernel64.bin
elf_file=kernel64.elf
image_target=myOS.img

for arg in "$@"
do
    if [[ $arg == "mode=32" ]]; then
        kernel_target=kernel32.bin
        image_target=myOS32.img
        elf_file=kernel32.elf
    elif [[ $arg == "mode=64" ]]; then
        kernel_target=kernel64.bin
        image_target=myOS.img
    fi
done

PROJECT_PATH=""
echo $platform
if [[ $platform == "platform=Mac" ]];
then
	PROJECT_PATH=${HOME}/Develop/myOS
    platform="Mac"
else
	PROJECT_PATH=${HOME}/dev/myOS
    platform="Linux"
fi
PROJECT_PATH=${HOME}/Develop/myOS

pushd $PROJECT_PATH

# Utility functions

READELF="/usr/local/opt/llvm/bin/llvm-readelf"

# Get the size of the kernel in blocks, rounded up.
get_kernel_size_in_blocks() {
    kernel_size=$(stat -f "%z" ${kernel_target})

    num_blocks=$(((kernel_size / 512) + 1)) 
    echo "kernel_size=$kernel_size,num_blocks=$num_blocks."
}

kernel_stack_size=0x100000
max_kernel_occupied_address=0
get_max_kernel_occupied_address() {
    section_addresses=`${READELF} --section-headers ${elf_file} | grep -E "( Address)" | cut -d":" -f2 | sed -e 's/^[[:space:]]*//'`
    section_sizes=`${READELF} --section-headers ${elf_file} | grep -E "( Size)" | cut -d":" -f2 | sed -e 's/^[[:space:]]*//'`

    section_addresses=($section_addresses)
    section_sizes=($section_sizes)

    num_sections=${#section_addresses[@]}
    count=0

    while [ $count -lt $num_sections ]
    do
        address=${section_addresses[$count]}
        size=${section_sizes[$count]}

        if [[ $((address + size)) -gt max_kernel_occupied_address ]];
        then
            max_kernel_occupied_address=$((address + size));
        fi
        count=$((count + 1))
    done
    echo $max_kernel_occupied_address
}

# The bootloader needs to know the kernel size (number of sectors)
# in order to be able to read the kernel into memory.
adjust_boot_loader() {
    # (Re-)Construct ${kernel_target} out of 512-byte blocks.
    # The dd command does not actually create an output file whose size is a
    # multiple of obs (512). So a TODO here might be to find a way to force
    # the output file to be a multiple of 512 (or whatever sector size is relevant).
    mv ${kernel_target} ${kernel_target}.tmp
    dd if=${kernel_target}.tmp of=${kernel_target} obs=512 conv=notrunc

    # Return value is in $num_blocks
    get_kernel_size_in_blocks

    # Put the right kernel size in the boot sector.
    uname=`uname -a`
    if [[ $uname == *"Darwin"* ]];
    then
        sed -i.swp "s/KERNEL_SIZE_SECTORS equ .*/KERNEL_SIZE_SECTORS equ $num_blocks/" boot/boot_sect_2.asm

        get_max_kernel_occupied_address
        dynamic_memory_start=$((max_kernel_occupied_address + kernel_stack_size))
        sed -i.swp "s/DYNAMIC_MEMORY_START equ .*/DYNAMIC_MEMORY_START equ $dynamic_memory_start/" boot/boot_sect_2.asm
    else
        sed -i "s/KERNEL_SIZE_SECTORS equ .*/KERNEL_SIZE_SECTORS equ $num_blocks/" boot/boot_sect_2.asm
    fi

    # Clean up
    rm -rf ${kernel_target}.tmp
}

# Generate the kernel itself. This includes boot sector(s) and actual kernel.
echo "***********************Generating Image******************************"

# We must build the kernel first - we need to know it's size so that the
# bootloader can load the correct amount of data from disk.
make ${kernel_target}-${platform}

# Adjust the bootloader to account for the kernel size.
adjust_boot_loader

# Build the final image ❤️ (bootloader + kernel) ❤️
make ${image_target}

echo "***********************Done generating Image*************************"

# Generate a disk containing a filesystem. The disk is called "disk.hdd".
echo "***********************Generating disk.hdd***************************"

BASH=/usr/local/Cellar/bash/5.2.15/bin/bash
${BASH} -x ${PROJECT_PATH}/scripts/generate_fs.sh

echo "***********************Done generating disk.hdd**********************"

popd
