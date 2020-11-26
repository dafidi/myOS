
void main(void) {
    asm volatile("add $0xbabe, %eax");
    int a = 1;
    while(a) { a = 0x1; }
}
