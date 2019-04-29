// A simple kernel which simply two characters at the top left of the screen.

void main() {
	char* video_memory = (char*) 0xb8000;

	video_memory[0] = 'Q'; 
	video_memory[2] = 'X';
}

