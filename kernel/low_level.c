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
	// "d: (port) means: load EDX with port
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

void insl(int port, void* addr, int cnt) {
	asm volatile("cld\n\trepne\n\tinsl"
	: "=D" (addr), "=c"(cnt)
	: "d" (port), "0" (addr), "1"(cnt)
	: "memory", "cc");
}
