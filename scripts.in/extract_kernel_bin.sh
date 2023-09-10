#!/bin/bash
#
# File: extract_kernel_bin.sh
#

# Stop on any error.
set -e
set -x

mode="mode=64"

ELF_FILE=""
BIN_FILE=""

if [[ $1 == "mode=32" ]];
then
    ELF_FILE="kernel32.elf"
    BIN_FILE="kernel32.bin"
else
    ELF_FILE="kernel64.elf"
    BIN_FILE="kernel64.bin"
fi

uname=`uname -a`

extract_linux() {
    objcopy -O binary $ELF_FILE $BIN_FILE
}

# These are the sections that typically exist:
# __TEXT,__text WANT
# __TEXT,__eh_frame DON'T WANT
# __TEXT,__const WANT
# __TEXT,__cstring WANT
# __DATA,__const WANT
# __DATA,__bss DON'T WANT
# __DATA,__common DON'T WANT
# __DATA,__data WANT
# __BSS,__bss DON'T WANT (empty anyways)
 
# TODO: Find a tool which can read ELF files and can be easily installed on
# MacOS.
READELF="/usr/local/opt/llvm/bin/llvm-readelf"
max_kernel_occupied_address=0

# NOTE: mac only.
add_section() {
    local name=$1
    local address=$2
    local offset=$3
    local size=$4

    if [[ $((address + size)) -gt max_kernel_occupied_address ]];
    then
        max_kernel_occupied_address=$((address + size));
    fi

    if [ "${name}" = '__bss' ]; then
        echo "(bss) returning for $name"
        return;
    fi
    if [[ "$name" = "__eh_frame" ]]
    then
        echo "(eh_frame) returning for $name"
        return;
    fi
    if [[ "$name" = "__common" ]]
    then
        echo " (common) returning for $name"
        return;
    fi

    iseek_blocks=$((offset))
    # iseek_blocks=$((iseek_bloxks / 512))
    oseek_blocks=$((address - 0x100000))
    # oseek_blocks=$((oseek_blocks / 512))
    echo iseek=${iseek_blocks} oseek=${oseek_blocks}
    dd iseek=${iseek_blocks} oseek=${oseek_blocks} if=${ELF_FILE} of=${BIN_FILE} bs=1 count=$((size)) conv=notrunc
}

# Based on Manual/Anectdotal inspection of the macho elf these are the sections 
# we want to copy. Skip everything else.
extract_mac() {
    section_names=`${READELF} --section-headers ${ELF_FILE} | grep -E "( Name)" | awk -F'[:(]' '{ print $2 }'`
    section_segments=`${READELF} --section-headers ${ELF_FILE} | grep -E "( Segment)" | awk -F'[:(]' '{ print $2 }'`
    section_addresses=`${READELF} --section-headers ${ELF_FILE} | grep -E "( Address)" | cut -d":" -f2 | sed -e 's/^[[:space:]]*//'`
    section_offsets=`${READELF} --section-headers ${ELF_FILE} | grep -E "( Offset)" | cut -d":" -f2 | sed -e 's/^[[:space:]]*//'`
    section_sizes=`${READELF} --section-headers ${ELF_FILE} | grep -E "( Size)" | cut -d":" -f2 | sed -e 's/^[[:space:]]*//'`

    # Convert to arrays.
    section_names=($section_names)
    section_segments=($section_segments)
    section_addresses=($section_addresses)
    section_offsets=($section_offsets)
    section_sizes=($section_sizes)

    num_sections=${#section_names[@]}
    count=0

    while [ $count -lt $num_sections ]
    do
        echo ${section_names[$count]};
        name=${section_names[$count]}
        address=${section_addresses[$count]}
        offset=${section_offsets[$count]}
        size=${section_sizes[$count]}

        add_section $name $address $offset $size

        count=$((count + 1))
    done
}

# extract_windows() {
#     #TODO: may never do.
# }

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
