#!/bin/bash
#
# File: generate_fs.sh
#
# Script to pretentiously a valid fs as defined by fnode.h.
# apps/app.bin must be present.

# Stop on any error.
set -e
# Show commands.
set -x

# Build binary which will live in filesystem.
make -C apps s
if [ ! -f apps/app.bin ];
then
    echo No app.bin found. Exiting.
    exit
fi

# Get the size of the one file that will be in the filesystem. The 
# filesystem needs this information.
APP_SIZE=$(stat apps/app.bin | grep Size | cut -d' ' -f4)

declare -A FILESYSTEM_SETTINGS

FILESYSTEM_PARAMS=(\
BITS_PER_BYTE \
TOTAL_DISK_SIZE \
SECTOR_SIZE \
FNODE_SIZE \
MASTER_RECORD_SIZE \
)

TEMPLATE_LOCATION=templates/filesystem
METADATA_SIZE=0
METADATA_SIZE_SECTORS=0

calculate_filesystem_configuration() {
    # Get user-set params.
    for param in ${FILESYSTEM_PARAMS[@]};
    do
        value=`awk -v p="${param}" '$0~p{print $2}' settings/filesystem.settings`
        FILESYSTEM_SETTINGS[$param]=$value
    done

    # Calculate derived params based on user settings. No validation, users should please set sensible params.
    FILESYSTEM_SETTINGS[NUM_SECTORS]=$((FILESYSTEM_SETTINGS[TOTAL_DISK_SIZE] / FILESYSTEM_SETTINGS[SECTOR_SIZE]))
    FILESYSTEM_SETTINGS[SECTOR_BITMAP_SIZE]=$((FILESYSTEM_SETTINGS[NUM_SECTORS] / FILESYSTEM_SETTINGS[BITS_PER_BYTE]))
    FILESYSTEM_SETTINGS[SECTOR_BITMAP_SIZE_IN_SECTORS]=$((FILESYSTEM_SETTINGS[SECTOR_BITMAP_SIZE] / FILESYSTEM_SETTINGS[SECTOR_SIZE]))
    FILESYSTEM_SETTINGS[FNODES_PER_SECTOR]=$((FILESYSTEM_SETTINGS[SECTOR_SIZE] / FILESYSTEM_SETTINGS[FNODE_SIZE]))
    # This is calculating the theoretical max nmuber of fnodes that could be needed
    # and assuming that each file occupies at least 1 sector.
    FILESYSTEM_SETTINGS[NUMBER_OF_FNODES]=$((FILESYSTEM_SETTINGS[TOTAL_DISK_SIZE] / FILESYSTEM_SETTINGS[SECTOR_SIZE]))
    FILESYSTEM_SETTINGS[FNODE_TABLE_SIZE_IN_SECTORS]=$((FILESYSTEM_SETTINGS[NUMBER_OF_FNODES] / FILESYSTEM_SETTINGS[FNODES_PER_SECTOR]))
    FILESYSTEM_SETTINGS[FNODE_TABLE_SIZE]=$((FILESYSTEM_SETTINGS[FNODE_TABLE_SIZE_IN_SECTORS] * FILESYSTEM_SETTINGS[SECTOR_SIZE]))
    FILESYSTEM_SETTINGS[FNODE_BITMAP_SIZE]=$((FILESYSTEM_SETTINGS[NUMBER_OF_FNODES] / FILESYSTEM_SETTINGS[BITS_PER_BYTE]))
    FILESYSTEM_SETTINGS[FNODE_BITMAP_SIZE_IN_SECTORS]=$((FILESYSTEM_SETTINGS[FNODE_BITMAP_SIZE] / FILESYSTEM_SETTINGS[SECTOR_SIZE]))

    FILESYSTEM_SETTINGS[FILESYSTEM_METADATA_SIZE]=$((FILESYSTEM_SETTINGS[MASTER_RECORD_SIZE] + FILESYSTEM_SETTINGS[FNODE_BITMAP_SIZE] + FILESYSTEM_SETTINGS[SECTOR_BITMAP_SIZE] + FILESYSTEM_SETTINGS[FNODE_TABLE_SIZE]))

    for s in "${!FILESYSTEM_SETTINGS[@]}";
    do
        printf "[%s]=%s\n" "$s" "${FILESYSTEM_SETTINGS[$s]}";
    done
}

# The order of these components is important
FILESYSTEM_METADATA_COMPONENTS=(\
master_record           \
fnode_bitmap            \
sector_bitmap           \
fnode_table             \
)

# This function cannot *echo* randomly. Its output is consumed by caller.
build_metadata_component() {
    metadata_component=$1
    metadata_file=${BUILD_LOCATION}/${metadata_component}.asm
    component_bin=${BUILD_LOCATION}/${metadata_component}.bin

    if [ ! -f $component_bin ]
    then
        time nasm -f bin -o ${component_bin} ${metadata_file}
    else
        metadata_bin_size=`stat ${component_bin} | awk '/Size/{ print $2 }'`
        if [[ ${metadata_component} != "fnode_table" && ${metadata_bin_size} != ${component_size} ]]
        then
            print "Have mismatch on component ${metadata_component}: actual(${metadata_file_size}) != expected(${component_size})."
            print "Attempting to build."
            time nasm -f bin -o ${component_bin} ${metadata_file}
        fi
    fi

    metadata_bin_size=`stat ${component_bin} | awk '/Size/{ print $2 }'`

    case ${metadata_component} in

    master_record)
        component_size=${FILESYSTEM_SETTINGS[MASTER_RECORD_SIZE]}
    ;;

    fnode_bitmap)
        component_size=${FILESYSTEM_SETTINGS[FNODE_BITMAP_SIZE]}
    ;;

    sector_bitmap)
        component_size=${FILESYSTEM_SETTINGS[SECTOR_BITMAP_SIZE]}
    ;;

    fnode_table)
        component_size=${FILESYSTEM_SETTINGS[FNODE_TABLE_SIZE]}
    ;;

    esac

    # Skip this for the fnode_table component. Because it is really big (e.g. 4GiB for a 16GiB disk)
    # building it is very time-comsuming so the asm file only builds a few fnodes. This means the size
    # won't match the component_size here.
    if [[ ${metadata_component} != "fnode_table" && ${metadata_bin_size} != ${component_size} ]]
    then
        echo "Still have mismatch on component ${metadata_component}: actual(${metadata_file_size}) != expected(${component_size}) Exiting."
        exit 0
    fi

    echo $component_bin $component_size
}

build_fs_metadata() {
    BUILD_LOCATION=/tmp/myOS-build
    SECTOR_SIZE=${FILESYSTEM_SETTINGS[SECTOR_SIZE]}
    if [ ! -d ${BUILD_LOCATION} ]
    then
        mkdir ${BUILD_LOCATION}
    fi

    for metadata_component in ${FILESYSTEM_METADATA_COMPONENTS[@]};
    do
        metadata_file=${metadata_component}.asm
        cp ${TEMPLATE_LOCATION}/${metadata_file} ${BUILD_LOCATION}/

        # Transform the template as necessary.
        sed -i "s/APP_BIN_SIZE/${APP_SIZE}/g" ${BUILD_LOCATION}/${metadata_file}

        # Build the raw file from the transformed assembly file.
        result=$(build_metadata_component ${metadata_component})
        component_bin=`echo ${result} | cut -d' ' -f1`
        component_size=`echo ${result} | cut -d' ' -f2`

        # Copy the built file into the final disk at the correct location.
        dd if=${component_bin} of=disk.hdd obs=512 seek=$((METADATA_SIZE / SECTOR_SIZE)) conv=notrunc

        # Note the size of the added raw file.
        METADATA_SIZE=$((METADATA_SIZE + component_size))
    done
    METADATA_SIZE_SECTORS=$((METADATA_SIZE / SECTOR_SIZE))
}

build_fs_data() {
    data_bin=${BUILD_LOCATION}/data.fs
    data_template=data.asm

    cp ${TEMPLATE_LOCATION}/${data_template} ${BUILD_LOCATION}/
    data_file=${BUILD_LOCATION}/${data_template}
    sed -i "s/APP_BIN_SIZE/${APP_SIZE}/g" ${data_file}
    nasm -f bin -o ${data_bin} ${data_file}

    dd if=${data_bin} of=disk.hdd obs=512 seek=${METADATA_SIZE_SECTORS} conv=notrunc

    # TODO: Add this to data_bin.
    dd if=apps/app.bin of=disk.hdd obs=512 seek=$((METADATA_SIZE_SECTORS + 1)) conv=notrunc
    dd if=apps/app.bin of=disk.hdd obs=512 seek=$((METADATA_SIZE_SECTORS + 2)) conv=notrunc
}

calculate_filesystem_configuration

# Allocate 16 GiB on disk.
fallocate -l 16G disk.hdd

build_fs_metadata

build_fs_data

echo "Displaying fs content (1st 2048):"
hexdump -n 2048 -C disk.hdd
echo "Displaying fs content (skip ${METADATA_SIZE}):"
hexdump -s $((METADATA_SIZE)) -n 2048 -C disk.hdd
