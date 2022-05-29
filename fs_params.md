In designing the fs, the constraints for the system we are looking at are:

- bits_per_byte: 8
- total_disk_size: 16 GiB
- sector_size: 512 B
- fnode_size: 128 B

Implications:

number_of_sectors:
    total_disk_size / sector_size = (16*2^30) / 512 = 2^25
sector_bitmap_size:
    number_of_sectors / bits_per_byte = 2^25 / 8 = 2^22
number_of_sectors_spanned_by_sector_bitmap:
    sector_bitmap_size / sector_size = 2^22 / 512 = 2^13

number_of_fnodes_per_sector: (implicit goal of consuming no more than 25% of diskspace with fnodes.)
    sector_size / fnode_size = 512 / 128 = 4 
number_of_fnodes:
    total_disk_size / sector_size = (16*2^30) / 512 = 2^25
number_of_sectors_spanned_by_fnode_table:
    number_of_fnodes / number_of_fnodes_per_sector = 2^25 / 4 = 2^23
fnode_table_size:
    number_of_sectors_spanned_by_fnode_table * sector_size = 2^23 * 512 = 2**32 (LOL since we don't have this much RAM, we'll always read this from disk.)
fnode_bitmap_size:
    # This should be number_of_fnodes (not fnode_table_size)
    # but too much has been done using this so we'll leave it for now.
    fnode_table_size / 8 = 2^32 / 8 = 2^29
number_of_sectors_spanned_by_fnode_bitmap:
    fnode_bit_map_size / sector_size = 2^29 / 512 = 2^20
