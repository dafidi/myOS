
REM # Convenience script to start the VM.
qemu-system-i386 -boot c,menu=off -drive file=os-image,format=raw,if=ide,media=disk,index=0 -m 512M -drive file=storage_disk.img,format=raw,if=ide,media=disk,index=1
