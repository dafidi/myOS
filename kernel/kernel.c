// A simple kernel which simply let's you know that I'm the author.
#include "../drivers/screen.h"

void main() {
	char* hello_to_my_os_message = "Kernel loaded and now running!\n";
	print(hello_to_my_os_message);
}

