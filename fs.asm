; Assembly file to pretentiously generate filesystem. in concert with generate_fs.sh
;
; ---------- start of sector filesystem's 1st sector. --------- ;
db "master_record"
times 128 - ($ - $$) db 0x0
dd 262144 + 1
dd 0
dd 128 + 5 * 4
dd 1
times 512 - ($ - $$) db 0
; ---------- end of sector filesystem's 1st sector. --------- ;
db "root_folder"
times 512 + 128 - ($ - $$) db 0x0
dd 1
dd 262144 + 2
dd 0
dd 128 + 4 + 12 * 1
dd 0
times 1024 - ($ - $$) db 0
; ---------- end of sector filesystem's 2nd sector. ---------- ;
db "app.bin"
times 1024 + 128  - ($ - $$) db 0
dd 1
dd 262144 + 3
dd 0
; Two more lines will be added by generate_fs.sh. They will look like this:
; dd 0x_____ ; Adding the size of our fake file.
; times 1536 - ($ - \$$) db 0 ; Setting 0s up to byte 1536
