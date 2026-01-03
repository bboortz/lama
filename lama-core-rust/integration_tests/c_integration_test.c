#include <stdio.h>

extern bool lama_packet_callable();

int main() {
	bool result = lama_packet_callable();
	printf("Called Rust function, result: %d\n", result);
	if (result != 1) {
		printf("ERROR: Result NOT OK!\n");
		return 1;
	}

	printf("Result OK!\n");
	return 0;

}
