/**
 * Ops for reading data in from I/O ports. The underlying assembly
 * calls read data into/out of registers. 
 */
unsigned char port_byte_in(unsigned short);
unsigned char port_word_in(unsigned short);
void port_byte_out(unsigned short, unsigned char);
void port_word_out(unsigned short, unsigned char);

/**
 * Ops for reading data in from I/O ports. The underlying assembly
 * calls read data into/out of memory locations. 
 */
void insb(unsigned short port, void *buf, int nr);
void insw(unsigned short port, void *buf, int nr);
void insl(unsigned short port, void *buf, int nr);
void outsb(unsigned short port, void *buf, int nr);
void outsw(unsigned short port, void *buf, int nr);
void outsl(unsigned short port, void *buf, int nr);
