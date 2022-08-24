;-------------------------------------------------------------------------------------------------;
;                   |-------------|
;                   | fnode_table |
;                   |-------------|
DATA_BLOCKS_START_SECTOR equ 0x804001
dd 424                                  ; root fnode.size
dd 1                                    ; root fnode.type
times 60 db 0                           ; root fnode.reserved
dd DATA_BLOCKS_START_SECTOR             ; root sector_indexes[0]
times 14 dd 0                           ; root sector_indexes[1-14]
dd APP_BIN_SIZE                         ; app.bin fnode.size
dd 0                                    ; app.bin fnode.type
times 60 db 0                           ; app.bin fnode.reserved
dd DATA_BLOCKS_START_SECTOR + 1         ; app.bin sector_indexes[0]
times 14 dd 0                           ; app.bin sector_indexes[1-14]
dd APP_BIN_SIZE                         ; app.bin2 fnode.size
dd 0                                    ; app.bin2 fnode.type
times 60 db 0                           ; app.bin2 fnode.reserved
dd DATA_BLOCKS_START_SECTOR + 2         ; app.bin2 indexes[0]
times 14 dd 0                           ; app.bin2 indexes[1-14]
;-------------------------------------------------------------------------------------------------;
