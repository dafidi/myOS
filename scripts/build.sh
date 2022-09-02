#!/bin/bash
#
# File: build.sh
# 
# This ought to be run in a Linux shell.

# Stop on any error.
set -e

cd ${HOME}/dev/myOS

# Utility functions.

# Get the size of the kernel in blocks, rounded up.
get_kernel_size_in_blocks() {
    kernel_size=$(stat kernel.bin | grep Size | tr -s ' ' | cut -d' ' -f3)

    num_blocks=$((($kernel_size / 512) + 1)) 
    echo "kernel_size=$kernel_size,num_blocks=$num_blocks."
}

# The bootloader needs to know the kernel size (number of sectors)
# in order to be able to read the kernel into memory.
adjust_boot_loader() {
    # (Re-)Construct kernel.bin out of 512-byte blocks.
    # The dd command does not actually create an output file whose size is a
    # multiple of obs (512). So a TODO here might be to find a way to force
    # the output file to be a multiple of 512 (or whatever sector size is relevant).
    mv kernel.bin kernel.bin.tmp
    dd if=kernel.bin.tmp of=kernel.bin obs=512 conv=notrunc

    # Return value is in $num_blocks
    get_kernel_size_in_blocks

    # Put the right kernel size in the boot sector.
    sed -i "s/KERNEL_SIZE_SECTORS equ .*/KERNEL_SIZE_SECTORS equ $num_blocks/" boot/boot_sect_2.asm

    # Clean up
    rm -rf kernel.bin.tmp
}

# Generate the kernel itself. This includes boot sector(s) and actual kernel.
echo "***********************Generating Image******************************"

# We must build the kernel first - we need to know it's size so that the
# bootloader can load the correct amount of data from disk.
make kernel.bin

# Adjust the bootloader to account for the kernel size.
adjust_boot_loader

# Build the final image ❤️ (bootloader + kernel) ❤️
make

echo "***********************Done generating Image*************************"

# Generate a disk containing a filesystem. The disk is called "disk.hdd".
echo "***********************Generating disk.hdd***************************"

source ${HOME}/dev/myOS/scripts/generate_fs.sh

echo "***********************Done generating disk.hdd**********************"
