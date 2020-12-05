unsigned char port_byte_in(unsigned short);

void port_byte_out(unsigned short, unsigned char);

unsigned char port_word_in(unsigned short);

void port_word_out(unsigned short, unsigned char);

#define insb(port, buf, nr) \
__asm__ ("cld\nrep insb\n\t"	\
::"d"(port), "D"(buf), "c"(nr))

#define insw(port, buf, nr) \
__asm__ ("cld\nrep insw\n\t"	\
::"d"(port), "D"(buf), "c"(nr))

#define outsw(port, buf, nr) \
__asm__ ("cld\nrep outsw\n\t"	\
::"d"(port), "S"(buf), "c"(nr))

void insl(int port, void* addr, int cnt);
