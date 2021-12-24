"""
sector_math.py - Utility functions for calculating file-system related parameters such as:

Given disk size,

    i)      how many sectors do we have?
    ii)     how many sector_descriptors do we need?
    iii)    how much memory is needed to store sector_descriptors?
    iv)     how many sectors are needed to store the sector_descriptors?
    v)      how many sector_descriptors are needed for the sectors used by the sector_descriptors?
    vi)     how much memory is needed to store the sector_descriptors for sectors occupied by the
            sector_descriptors?
    vii)    how many sectors are needed to store the memory used by the sector_descriptors that
            describe the sectors used by all the sector_descriptors?
"""

import argparse

KiB = 1024
MiB = 1024 * KiB
GiB = 1024 * MiB

DEFAULT_DISK_SIZE = 16 * GiB
DEFAULT_SECTOR_SIZE_BYTES = 512
DEFAULT_SECTOR_SIZE_SHIFT = 9
# A descriptor here means a 4-byte object which holds information about a sector
# in this way: bits: | 31 ... 16 | 15 ... 0 | 
#                    |  reserved |  filled  |
#
DEFAULT_DESCRIPTOR_SIZE_BYTES = 4

class DiskParams:
    def __init__(self, disk_size=DEFAULT_DISK_SIZE,
                 sector_size_bytes=DEFAULT_SECTOR_SIZE_BYTES):
        self.disk_size = disk_size
        self.sector_size_bytes = sector_size_bytes
    
    def disk_size(self):
        return self.disk_size

    def sector_size_bytes(self):
        return self.sector_size_bytes

class MetaParameters:
    def __init__(self):
        self.num_sectors = 0
        self.num_sector_descriptors = 0
        self.sector_descriptors_memory = 0
        self.num_sector_descriptors_sectors = 0
        self.num_sector_descriptors_descriptors = 0
        self.sector_descriptors_descriptors_memory = 0
        self.num_sector_descriptors_descriptors_sectors = 0

    def __str__(self):
        return (
"""
num_sectors: %s
num_sector_descriptors: %s
sector_descriptors_memory: %s
num_sector_descriptors_sectors: %s
num_sector_descriptors_descriptors: %s
sector_descriptors_descriptors_memory: %s
num_sector_descriptors_descriptors_sectors: %s
""" % (
self.num_sectors,
self.num_sector_descriptors,
self.sector_descriptors_memory,
self.num_sector_descriptors_sectors,
self.num_sector_descriptors_descriptors,
self.sector_descriptors_descriptors_memory,
self.num_sector_descriptors_descriptors_sectors)
)

    def set_num_sectors(self, n):
        self.num_sectors = n
    
    def set_num_sector_descriptors(self, n):
        self.num_sector_descriptors = n
    
    def set_sector_descriptors_memory(self, n):
        self.sector_descriptors_memory = n
    
    def set_num_sector_descriptors_sectors(self, n):
        self.num_sector_descriptors_sectors = n
    
    def set_num_sector_descriptors_descriptors(self, n):
        self.num_sector_descriptors_descriptors = n
    
    def set_sector_descriptors_descriptors_memory(self, n):
        self.sector_descriptors_descriptors_memory = n
    
    def set_num_sector_descriptors_descriptors_sectors(self, n):
        self.num_sector_descriptors_descriptors_sectors = n

def describe_meta_parameters(disk_size=None):
    """
    Calculates and prints out data described in file description.
    """

    if disk_size is None:
        print("No disk_size provided.")
        return

    meta_parameters = MetaParameters()
    disk_params = DiskParams(disk_size=disk_size)

    meta_parameters.set_num_sectors(disk_params.disk_size >> DEFAULT_SECTOR_SIZE_SHIFT)
    meta_parameters.set_num_sector_descriptors(meta_parameters.num_sectors)
    meta_parameters.set_sector_descriptors_memory(
        meta_parameters.num_sector_descriptors * DEFAULT_DESCRIPTOR_SIZE_BYTES)
    meta_parameters.set_num_sector_descriptors_sectors(
        meta_parameters.sector_descriptors_memory >> DEFAULT_SECTOR_SIZE_SHIFT)
    meta_parameters.set_num_sector_descriptors_descriptors(
        meta_parameters.num_sector_descriptors_sectors)
    meta_parameters.set_sector_descriptors_descriptors_memory(
        meta_parameters.num_sector_descriptors_descriptors * DEFAULT_DESCRIPTOR_SIZE_BYTES)
    meta_parameters.set_num_sector_descriptors_descriptors_sectors(
        meta_parameters.sector_descriptors_descriptors_memory >> DEFAULT_SECTOR_SIZE_SHIFT)  

    print(meta_parameters)

def disk_size_unit_arg_parse(arg):
    map = {'KiB' : KiB, 'MiB': MiB, 'GiB' : GiB}
    try:
        v = map[arg]
        return v
    except:
        raise Exception('Unrecognized unit: %s' % arg)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--disk-size", type=int)
    parser.add_argument("--disk-size-unit", type=str)

    args = parser.parse_args()
    
    disk_size = args.disk_size
    disk_size_unit = args.disk_size_unit

    disk_size = disk_size * disk_size_unit_arg_parse(disk_size_unit)

    describe_meta_parameters(disk_size=disk_size)

if __name__ == "__main__":
    main()
