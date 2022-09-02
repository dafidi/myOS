#include "kernel/low_level.h"
#include "kernel/string.h"
#include "kernel/system.h"

#include "screen.h"

// Cursor things.
static int get_screen_offset(int row, int col) {
	return 2 * ((row)* MAX_COLS + col);
}

static int get_cursor(void) {
	// The device uses its control register as an index to select its
	// internal registers, of which we are interested in:
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

static void set_cursor(int offset) {
	offset /= 2;
	
	port_byte_out(REG_SCREEN_CTRL, 14);
	port_byte_out(REG_SCREEN_DATA, (unsigned char) (offset >> 8));
	port_byte_out (REG_SCREEN_CTRL, 15);
	port_byte_out(REG_SCREEN_DATA, offset);
}

/*  Advance  the  text  cursor , scrolling  the  video  buffer  if  necessary. */
static int handle_scrolling(int  cursor_offset) {
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

static void print_char(char character, int row, int col, char attribute_byte) {
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

void print_string(const char* message) {
	int i = 0;

	while(message[i] != 0) {
		int cursor_offset = get_cursor() - VIDEO_ADDRESS;
		int row = cursor_offset / (2 * MAX_COLS);
		int col = cursor_offset % (2 * MAX_COLS);
		print_char(message[i++], row, col, WHITE_ON_BLACK);
	}
}

// TODO: Use unsigned long long here.
void print_int(unsigned int n) {
    int num_digits = 0;

    while (n) {
        n /= 10;
        num_digits++;
    }

    if (num_digits == 0) {
        print_string("0");
        return;
    }

    char out_buff[num_digits + 1];
    out_buff[num_digits] = '\0';
    while (num_digits--) {
        int n_0unit = (n / 10) * 10;

        out_buff[num_digits] = '0' + n - n_0unit;

        n /= 10;
    }

    print_string(out_buff);
}

void print_int64(uint64_t n) {
    print_int(n);
}

void print_int32(int n) {
    print_int(n);
}

void print_ptr(void *p) {
    // TODO: Print as actual pointer, not int32.
    print_int64((uint32_t) p);
}

static void print_register(char *register_name, const int register_value) {
	print_string(register_name); print_string("="); print_int32(register_value); print_string("\n");
}

void print_registers(struct registers *registers) {
	print_register("gs", registers->gs);
	print_register("fs", registers->fs);
	print_register("es", registers->es);
	print_register("ds", registers->ds);
	print_register("edi", registers->edi);
	print_register("esi", registers->esi);
	print_register("ebp", registers->ebp);
	print_register("esp", registers->esp);
	print_register("ebx", registers->ebx);
	print_register("edx", registers->edx);
	print_register("ecx", registers->ecx);
	print_register("eax", registers->eax);
	print_register("eip", registers->eip);
	print_register("cs", registers->cs);
	print_register("useresp", registers->useresp);
	print_register("ss", registers->ss);
}
