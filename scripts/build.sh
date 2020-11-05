#!/bin/bash
#
# File: build.sh
# 
# This ought to be run in a Linux shell.

set -e
set -x

cd ${HOME}/dev/myOS

# Utility functions.
get_raw_kernel_size () {
  kernel_size=$(stat kernel.bin | grep Size | tr -s ' ' | cut -d' ' -f3)
  kernel_size=$(($kernel_size))
}

get_num_blocks_needed() {
  num_blocks=1

  echo "Looking for minimum number of sectors required for entire kernel."
  echo "Starting at num_blocks=$num_blocks"
  while [ $((512*$num_blocks)) -le $(($kernel_size)) ]
  do
    num_blocks=$(($num_blocks + 1))
  done
  echo "Finished! Needed num_blocks=$num_blocks."
}

pad_kernel_to_multiple_of_sector_size() {
  padding=$((512*$num_blocks - $kernel_size))
  echo "Padding Kernel with $padding bytes."

  echo "times $padding db 0" > pad.asm
  nasm -f bin -o padding.bin pad.asm

  cat kernel.bin padding.bin > padded_kernel.bin
  cat padded_kernel.bin > kernel.bin
  num_blocks=$num_blocks
}

echo "***********************Generating os-image***************************"
make kernel.bin

# Figure out numbers of sectors to load in boot sector.
get_raw_kernel_size
get_num_blocks_needed

# Padding is just to make the kernel size a multiple of 512 (sector size).
pad_kernel_to_multiple_of_sector_size

# Put the right kernel size in the boot sector.
sed -i "s/KERNEL_SIZE_SECTORS equ .*/KERNEL_SIZE_SECTORS equ $num_blocks/" boot/boot_sect_2.asm

make boot_sect_1.bin
make boot_sect_2.bin
make storage_disk.img
make os-image
echo "***********************Done generating os-image**********************"

