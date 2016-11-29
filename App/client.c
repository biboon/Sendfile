#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <libcom.h>

int main(int argc, char const *argv[]) {
	int fd = com_tcp_connect(argv[1], argv[2]);
	if (fd == -1) return -1;

	printf("Connected to server!\n");
	size_t size = 12;
	com_write(fd, &size, sizeof(size), -1);

	com_close(fd);

	return 0;
}
