#!/bin/bash

MAC_OBJCOPY=/usr/local/opt/llvm/bin/llvm-objcopy
LINUX_OBJCOPY=objcopy

extract_mac() {
    ${MAC_OBJCOPY} -O binary --only-section=.text app.s.elf app.bin
}

extract_linux() {
    ${LINUX_OBJCOPY} -O binary --only-section=.text app.s.elf app.bin
}

PLATFORM=`uname -a | cut -d" " -f1`
if [[ $PLATFORM == "Linux" ]]
then
    extract_linux;
elif [[ $PLATFORM = "Darwin" ]]
then
    extract_mac
else
    echo "Unknown platform. Doing nothing."
fi
