;-------------------------------------------------------------------------------------------------;
;                   |-------------|
;                   | fnode_table |
;                   |-------------|
DATA_BLOCKS_START_SECTOR equ 0x804001
dd 0                                    ; [ root     ] fnode.id
dd 424                                  ; [ root     ] fnode.size
dd 1                                    ; [ root     ] fnode.type
times 56 db 0                           ; [ root     ] fnode.reserved
dd DATA_BLOCKS_START_SECTOR             ; [ root     ] sector_indexes[0]
times 14 dd 0                           ; [ root     ] sector_indexes[1-14]
dd 1                                    ; [ app.bin  ] fnode.id
dd APP_BIN_SIZE                         ; [ app.bin  ] fnode.size
dd 0                                    ; [ app.bin  ] fnode.type
times 56 db 0                           ; [ app.bin  ] fnode.reserved
dd DATA_BLOCKS_START_SECTOR + 1         ; [ app.bin  ] fnode.sector_indexes[0]
times 14 dd 0                           ; [ app.bin  ] fnode.sector_indexes[1-14]
dd 2                                    ; [ app2.bin ] fnode.id
dd APP_BIN_SIZE                         ; [ app2.bin ] fnode.size
dd 0                                    ; [ app2.bin ] fnode.type
times 56 db 0                           ; [ app2.bin ] fnode.reserved
dd DATA_BLOCKS_START_SECTOR + 2         ; [ app2.bin ] fnode.sector_indexes[0]
times 14 dd 0                           ; [ app2.bin ] fnodesector_indexes[1-14]
;-------------------------------------------------------------------------------------------------;
