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

PROJECT_PATH=UNCONFIGURED_PROJECT_PATH
KERNEL_BIN_TARGET=UNCONFIGURED_KERNEL_BIN_TARGET
IMAGE_TARGET=UNCONFIGURED_IMAGE_TARGET
ELF_FILE=kernel64.elf
KERNEL_TARGET=kernel64.bin

kernel_stack_size=0x100000
max_kernel_occupied_address=0


PLATFORM=`uname -a | cut -d" " -f1`
if [[ $PLATFORM == "Linux" ]]
then
    READELF="readelf"
elif [[ $PLATFORM = "Darwin" ]]
then
    READELF="/usr/local/opt/llvm/bin/llvm-readelf"
else
    echo "Unknown platform. Quitting."
    exit
fi

# Utility functions

# Get the size of the kernel in blocks, rounded up.
get_kernel_size_in_blocks() {
     uname=`uname -a`
    if [[ $uname == *"Darwin"* ]];
    then
        kernel_size=$(stat -f "%z" ${KERNEL_TARGET})
    else
        kernel_size=$(stat -c "%s" ${KERNEL_TARGET})
    fi

    num_blocks=$(((kernel_size / 512) + 1)) 
    echo "kernel_size=$kernel_size,num_blocks=$num_blocks."
}

# Get the max address used by code/data from the headers in ELF_FILE.
get_max_kernel_occupied_address() {
    if [[ $PLATFORM == "Linux" ]]
    then
        section_addresses=`${READELF} --program-header ${ELF_FILE} | grep -E "LOAD[ ]*0x[0-9a-e]* 0x[0-9a-e]* 0[0-9a-e]*" | tr -s " " | cut -d" " -f4`
        section_sizes=`${READELF} --program-header ${ELF_FILE} | grep -E "0x[0-9a-e]* 0x[0-9a-e]*[ ]*(R|RW|RWE)" | tr -s " " | cut -d" " -f3`
    elif [[ $PLATFORM = "Darwin" ]]
    then
        section_addresses=`${READELF} --section-headers ${ELF_FILE} | grep -E "( Address)" | cut -d":" -f2 | sed -e 's/^[[:space:]]*//'`
        section_sizes=`${READELF} --section-headers ${ELF_FILE} | grep -E "( Size)" | cut -d":" -f2 | sed -e 's/^[[:space:]]*//'`
    else
        echo "Unknown platform. Doing nothing."
    fi

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
    mv ${KERNEL_TARGET} ${KERNEL_TARGET}.tmp
    dd if=${KERNEL_TARGET}.tmp of=${KERNEL_TARGET} obs=512 conv=notrunc

    # Return value is in $num_blocks
    get_kernel_size_in_blocks

    get_max_kernel_occupied_address
    dynamic_memory_start=$((max_kernel_occupied_address + kernel_stack_size))

    # Put the right kernel size & max address in the boot sector.
    uname=`uname -a`
    if [[ $uname == *"Darwin"* ]];
    then
        sed -i.swp "s/KERNEL_SIZE_SECTORS equ .*/KERNEL_SIZE_SECTORS equ $num_blocks/" boot/boot_sect_2.asm
        sed -i.swp "s/MAX_KERNEL_STATIC_MEM equ .*/MAX_KERNEL_STATIC_MEM equ $dynamic_memory_start/" boot/boot_sect_2.asm
    else
        sed -i "s/KERNEL_SIZE_SECTORS equ .*/KERNEL_SIZE_SECTORS equ $num_blocks/" boot/boot_sect_2.asm
        sed -i "s/MAX_KERNEL_STATIC_MEM equ .*/MAX_KERNEL_STATIC_MEM equ $dynamic_memory_start/" boot/boot_sect_2.asm
    fi

    # Clean up
    rm -rf ${KERNEL_TARGET}.tmp
}

for arg in "$@"
do
    if [[ $arg == "mode=32" ]]; then
        KERNEL_TARGET=kernel32.bin
        IMAGE_TARGET=myOS32.img
        ELF_FILE=kernel32.elf
    fi
done

if [[ $PROJECT_PATH == UNCONFIGURED_PROJECT_PATH ]];
then
    echo "This project is unconfigured. You must run .configure first."
	echo ".configure figures out the parameters for your environment."
    exit 0
fi

PLATFORM=`uname -a | cut -d" " -f1`
if [[ $PLATFORM == "Linux" ]]
then
    READELF=readelf
elif [[ $PLATFORM = "Darwin" ]]
then
    READELF=/usr/local/opt/llvm/bin/llvm-readelf
else
    echo "Unknown platform. Unable to use readelf. Exiting."
    exit
fi

pushd $PROJECT_PATH

# Generate the kernel itself. This includes boot sector(s) and actual kernel.
echo "***********************Generating Image******************************"

# We must build the kernel first - we need to know it's size so that the
# bootloader can load the correct amount of data from disk.
make ${KERNEL_BIN_TARGET}

# Adjust the bootloader to account for the kernel size.
adjust_boot_loader

# Build the final image ❤️ (bootloader + kernel) ❤️
make ${IMAGE_TARGET}

echo "***********************Done generating Image*************************"

# Generate a disk containing a filesystem. The disk is called "disk.hdd".
echo "***********************Generating disk.hdd***************************"

# Ensure that bash version is >= 5.2.15
bash -x ${PROJECT_PATH}/scripts/generate_fs.sh

echo "***********************Done generating disk.hdd**********************"

popd
