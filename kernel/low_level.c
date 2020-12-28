unsigned char port_byte_in(unsigned short port) {
	// Reads a byte from the specified port.
	// "=a" (result) means: put AL register in variable RESULT when finished
	// "d" (port) means: load EDX with port
	unsigned char result;
	__asm__("in %%dx, %%al" : "=a" (result) : "d" (port));
	return result;
}

void port_byte_out(unsigned short port, unsigned char data) {
	// "a" (data) means: load EAX with data
	// "d" (port) means: load EDX with port
	__asm__("out %%al, %%dx" : : "a" (data), "d" (port));
}

unsigned short port_word_in(unsigned short port) {
	unsigned short result;
	__asm__("in %%dx, %%ax" : "=a" (result) : "d" (port));
	return result;
}

void port_word_out(unsigned short port, unsigned char data) {
	__asm__("out %%al, %%dx" : : "a" (data), "d" (port));
}

/**
 * insb - Read nr bytes from port specified by port into starting address
 * specified by buf.
 * 
 * @port: I/O port to be read from, the insb instruction expects the port
 * to be specified in EDX so we put it in there.
 * @buf: The physical address  of the buffer into which data should be read
 * insb expect it to be in EDI. It is put into ES:EDI.
 * @nr: The number of words intended to be read. This directly translates
 * to the number of times the insb instruction will be executed by rep
 * which expects this to be specified in ECX.
 */
void insb(unsigned short port, void *buf, int nr) {
	asm volatile("cld\n\trep insb"::"d"(port), "D"(buf), "c"(nr));
}

/**
 * insw - Word variant of insb. See insb comment for full description.
 * Read nr words from port specified by port into starting address
 * specified by buf.
 * 
 * @port: I/O port to be read from.
 * @buf: The physical address of the buffer into which data should be read.
 * @nr: The number of words intended to be read.
 */
void insw(unsigned short port, void *buf, int nr) {
	asm volatile("cld\n\trep insw"::"d"(port), "D"(buf), "c"(nr));
}

/**
 * insl - Double-word (unconfirmed) variant of insb. See insb comment for
 * full description. Read nr words from port specified by port into starting
 * address specified by buf.
 * 
 * @port: I/O port to be read from.
 * @buf: The physical address of the buffer into which data should be read.
 * @nr: The number of words intended to be read.
 */
void insl(unsigned short port, void *buf, int nr) {
	asm volatile("cld\n\trep insl"::"d"(port), "D"(buf), "c"(nr));
}

/**
 * outsb - Write "nr" bytes into I/O port specified by "port" from the memory address
 * specified by "buf".
 * 
 * @port: I/O port to be written into.
 * outsb expects the port to be specified in EDX.
 * @buf: The physical address of the buffer containing data to write.
 * outsb expects the address to be in ESI.
 * @nr: The number of words to be written. This directly translates
 * to the number of times the outsb instruction will be executed by rep
 * which expects this to be specified in ECX.
 */
void outsb(unsigned short port, void *buf, int nr) {
	asm volatile("cld\n\trep outsb\n\t"::"d"(port), "S"(buf), "c"(nr));
}

/**
 * outsw - Write "nr" words into I/O port specified by "port" from the memory address
 * specified by "buf".
 * 
 * @port: I/O port to be written into.
 * outsw expects the port to be specified in EDX.
 * @buf: The physical address of the buffer containing data to write.
 * outsw expects it to be in ESI.
 * @nr: The number of words to be written. This directly translates
 * to the number of times the outsw instruction will be executed by rep
 * which expects this to be specified in ECX.
 */
void outsw(unsigned short port, void *buf, int nr) {
	asm volatile("cld\n\trep outsw\n\t"::"d"(port), "S"(buf), "c"(nr));
}

/**
 * outsl - Write "nr" double-words into I/O port specified by "port" from the memory address
 * specified by "buf".
 * 
 * @port: I/O port to be written into.
 * outsl expects the port to be specified in EDX.
 * @buf: The physical address of the buffer containing data to write.
 * outsl expects it to be in ESI.
 * @nr: The number of words to be written. This directly translates
 * to the number of times the outsl instruction will be executed by rep
 * which expects this to be specified in ECX.
 */
void outsl(unsigned short port, void *buf, int nr) {
	asm volatile("cld\n\trep outsl\n\t"::"d"(port), "S"(buf), "c"(nr));
}
