// A simple kernel which simply two characters at the top left of the screen.
#include "../drivers/screen.h"

void main() {
	char* hello_to_my_os_message = "Hello to \nDavid's O/S.";
	print(hello_to_my_os_message);
	print(hello_to_my_os_message);
	print(hello_to_my_os_message);
}

