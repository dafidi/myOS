#!/bin/bash
echo "***********************Building kernel*************************"
make kernel.bin
make storage_disk.img


make os-image
echo "***********************Done building kernel********************"

IFS=' '
last_field=''
kernel_size=''

stat_output=`stat os-image | grep Size`

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
num_blocks=1

echo "Looking for minimum number of sectors required for entire kernel."
echo "Starting at num_blocks=$num_blocks"
while [ $((512*$num_blocks)) -le $(($kernel_size)) ]
do
  num_blocks=$(($num_blocks + 1))
done
echo "Finished! Needed num_blocks=$num_blocks."

padding=$((512*$num_blocks - $kernel_size))
echo "Padding Kernel with $padding bytes."

echo "times $padding db 0" > pad.asm
nasm -f bin -o pad.bin pad.asm
nasm -f bin -o null.bin null.asm

cat os-image pad.bin > padded_kernel.bin
cat padded_kernel.bin null.bin > os-image
# cat padded_kernel.bin > os-image

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

