#include "kernel/low_level.h"
#include "screen.h"

void print_char(char character, int row, int col, char attribute_byte) {
	unsigned char* vidmem = (unsigned char*) VIDEO_ADDRESS;

	if (!attribute_byte) {
		attribute_byte = WHITE_ON_BLACK;
	}

	int offset;
	if (row >= 0 && col >= 0) {
		offset = get_screen_offset(row, col);
	} else {
		offset = get_cursor();
	}
	
	if (offset  >= MAX_ROWS*MAX_COLS *2) {
		return;
	}
		
	if (character == '\n') {
		int rows = offset / (2*MAX_COLS) + 1;
		offset = get_screen_offset(rows, 0);
	} else {
		vidmem[offset] = character;
		vidmem[offset+1] = attribute_byte;
		offset += 2;
	}

	offset = handle_scrolling(offset);

	set_cursor(offset);
}

int get_screen_offset(int row, int col) {
	return 2 * ((row)* MAX_COLS + col);
}

int get_cursor() {
	// The device uses its control register as an index to select its
	// internal registeres, of which we are interested in:
	//	reg 14: which is the high byte of the cursor's offset
	//	reg 15: which is the low byte of the cursor's offset
	// Once the internal register has been selected, we may read or
	// write a byte on the data register.
	port_byte_out(REG_SCREEN_CTRL, 14);
	int offset = port_byte_in(REG_SCREEN_DATA) << 8;
	port_byte_out(REG_SCREEN_CTRL, 15);
	offset += port_byte_in(REG_SCREEN_DATA);

	return offset * 2;
}

void set_cursor(int offset) {
	offset /= 2;
	
	port_byte_out(REG_SCREEN_CTRL, 14);
	port_byte_out(REG_SCREEN_DATA, (unsigned char) (offset >> 8));
	port_byte_out (REG_SCREEN_CTRL, 15);
	port_byte_out(REG_SCREEN_DATA, offset);
}

/* Not Used. */
void print_at(char* message, int row, int col) {
	if (row >= 0 && col >= 0) {
		set_cursor(get_screen_offset(row, col));
	}

	int i = 0;
	while(message[i] != 0) {
		print_char(message[i++], col, row, WHITE_ON_BLACK);
	}
}

void print(char* message) {
	int i = 0;
	while(message[i] != 0) {
		int cursor_offset = get_cursor() - VIDEO_ADDRESS;
		int row = cursor_offset / (2 * MAX_COLS);
		int col = cursor_offset % (2 * MAX_COLS);
		print_char(message[i++], row, col, WHITE_ON_BLACK);
	}
}

/* Not Used. */
void clear_screen() {
	int row = 0;
	int col = 0;

	for (row = 0; row < MAX_ROWS; row++) {
		for (col = 0; col < MAX_ROWS; col++) {
			print_char(' ', col, row, WHITE_ON_BLACK);
		}
	}

	set_cursor(get_screen_offset(0, 0));
}

/* Copy  bytes  from  one  place  to  another. */
void  memory_copy(char* source , char* dest , int  no_bytes) {
	int i;
	for (i=0; i<no_bytes; i++) {
		*(dest + i) = *( source + i);
	}
}

/*  Advance  the  text  cursor , scrolling  the  video  buffer  if  necessary. */
int  handle_scrolling(int  cursor_offset) {
	// If the  cursor  is  within  the  screen , return  it  unmodified.
	if (cursor_offset  < MAX_ROWS*MAX_COLS *2) {
		return  cursor_offset;
	}
	
	/*  Shuffle  the  rows  back  one. */
	int i;
	for (i=1; i<MAX_ROWS; i++) {
		memory_copy((char*) get_screen_offset (i,0) + VIDEO_ADDRESS, (char*) get_screen_offset (i-1,0) + VIDEO_ADDRESS ,MAX_COLS *2);
	}
	
	/*  Blank  the  last  line by  setting  all  bytes  to 0 */
	char* last_line = (char*) get_screen_offset (MAX_ROWS -1, 0) + VIDEO_ADDRESS;
	for (i=0; i < MAX_COLS *2; i++) {
		last_line[i] = 0;
	}
	
	// Move  the  offset  back  one row , such  that it is now on the  last// row , rather  than  off the  edge of the  screen.
	// cursor_offset  -= 2* MAX_COLS;
	cursor_offset = (int) last_line - VIDEO_ADDRESS;
	
	//  Return  the  updated  cursor  position.
	return  cursor_offset;
}

/**/
void print_int(char* p) {
	while(*p == '0') { p++;	}
	
	if (*p == '\0')
		p--; // Need to be able to print 0.
	
	print(p);
}

// Doesn't really work, lol
void set_screen_to_blue(void) {
	unsigned char* p = (unsigned char*) 	0xa0000;

	while (((int)p) < 0xbffff) {
		*p = 1;
		p++;
	}
}