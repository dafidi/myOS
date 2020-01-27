#define VIDEO_ADDRESS 0xb8000
#define MAX_ROWS 25
#define MAX_COLS 80

#define WHITE_ON_BLACK 0x0f

#define REG_SCREEN_CTRL 0x3D4
#define REG_SCREEN_DATA 0x3D5

// Cursor things.
int get_screen_offset(int, int);
int get_cursor();
void set_cursor(int);

// Printing things.
void print(char*);
void print_int(char*);
void print_at(char*, int, int);
void print_char(char, int, int, char);

// Screenwide ops.
void clear_screen();
void set_screen_to_blue(void);

// Scrolling.
int  handle_scrolling(int  cursor_offset);
