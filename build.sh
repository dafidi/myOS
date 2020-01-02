#!/bin/bash

# Utility functions.
get_raw_kernel_size () {
  IFS=' '
  last_field=''
  kernel_size=''

  stat_output=`stat kernel.bin | grep Size`

  read -ra ADDR <<< "$stat_output"

  for i in "${ADDR[@]}"; do
    if [[ $last_field == 'Size:' ]]; then
      kernel_size="$i"
      echo "Found kernel size: $i"
      break
    fi
    last_field=$i
  done

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

echo "***********************Building image*************************"
make kernel.bin

# Figure out numbers of sectors to load in boot sector.
get_raw_kernel_size
get_num_blocks_needed
# Padding is just to make the kernel size a multiple of 512 (sector size).
pad_kernel_to_multiple_of_sector_size
sed -i "s/KERNEL_SIZE_SECTORS equ .*/KERNEL_SIZE_SECTORS equ $num_blocks/" boot/boot_sect.asm

make boot_sect.bin
make storage_disk.img

make os-image
echo "***********************Done building Image********************"

# Convenience script to start the VM.
echo "****************************************************************"
echo "Launching Windows shell from which VM (qemu) will be launched..."
cmd.exe /c start.bat
echo "VM shut down."
echo "****************************************************************"

if [[ $1 == "--no-clean" ]]; then
  echo "Ha! I'm not cleaning my mess."
else
  echo "Cleaning up my mess."
  make clean
fi
echo "Done."

