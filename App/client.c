#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <libcom.h>

int main(int argc, char const *argv[]) {
	int fd = com_connect_stream(argv[1], argv[2]);
	if (fd == -1) return -1;

	printf("Connected to server!\n");
	size_t size = 50 * 1024 * 1024 * sizeof(char);
	unsigned char *buffer = malloc(size);
	com_write(fd, buffer, size, -1);
	printf("Write result: %zu\n", size);

	com_close(fd);

	return 0;
}
