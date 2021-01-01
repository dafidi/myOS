/**
 * Ops for reading data in from I/O ports. The underlying assembly
 * calls read data into/out of registers. 
 */
unsigned char inline  port_byte_in(unsigned short);
unsigned char inline port_word_in(unsigned short);
void inline port_byte_out(unsigned short, unsigned char);
void inline port_word_out(unsigned short, unsigned char);

/**
 * Ops for reading data in from I/O ports. The underlying assembly
 * calls read data into/out of memory locations. 
 */
void inline insb(unsigned short port, void *buf, int nr);
void inline insw(unsigned short port, void *buf, int nr);
void inline insl(unsigned short port, void *buf, int nr);
void inline outsb(unsigned short port, void *buf, int nr);
void inline outsw(unsigned short port, void *buf, int nr);
void inline outsl(unsigned short port, void *buf, int nr);
