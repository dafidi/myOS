
// #include <screen/screen.h>

void main(void) {
    asm volatile("add %eax, %ebx");
    while(1);
    return 0;
}
