;-------------------------------------------------------------------------------------------------;
; See fs_params.md for how values are obtained.
;
; SillyFS layout:
;
; | master_record | fnode_bitmap | sector_bitmap | fnode_table | data_blocks |
;
;---------------start of block/sector  0----------;
;                   |-------------|
;                   |master_record|
;                   |-------------|
dd 0                                    ; root_dir_fnode_location.fnode_table_index
dd 0x1 + 0x2000 + 0x2000                ; root_dir_fnode_location.fnode_sector_index
dw 0                                    ; root_dir_fnode_location.fnode_sector_offset
dd 1                                    ; fnode_bitmap_start_sector  (1)
dd 0x400000                             ; fnode_bitmap_size          (2^22)
dd 0x1 + 0x2000 + 0x2000                ; fnode_table_start_sector   (1 + 2^13 + 2^13)
dd 0x1 + 0x2000                         ; sector_bitmap_start_sector (1 + 2^13)
dd 0x400000                             ; sector_bitmap_size         (2^22)
dd 0x1 + 0x2000 + 0x2000 + 0x800000     ; data_blocks_start_sector   (1 + 2^13 + 2^13 + 2^32)
times 512 - ($ - $$) db 0
;-------------------------------------------------------------------------------------------------;
