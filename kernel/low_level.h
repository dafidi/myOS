unsigned char port_byte_in(unsigned short);

void port_byte_out(unsigned short, unsigned char);

unsigned char port_word_in(unsigned short);

void port_word_out(unsigned short, unsigned char);

#define outsw(port, buf, nr) \
__asm__ ("cld\nrep outsw\n\t"	\
::"d"(port), "S"(buf), "c"(nr))

void inline insb(unsigned short port, void *buf, int nr);
void inline insw(unsigned short port, void *buf, int nr);
void inline insl(unsigned short port, void *buf, int nr);
