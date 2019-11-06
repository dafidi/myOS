unsigned char port_byte_in(unsigned short);

void port_byte_out(unsigned short, unsigned char);

unsigned char port_word_in(unsigned short);

void port_word_out(unsigned short, unsigned char);

#define insl(port, buf, nr) \
__asm__ ("cld;rep;insb\n\t"	\
::"d"(port), "D"(buf), "c"(nr))

