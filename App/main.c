#include <stdio.h>
#include <unistd.h>

#include <libcom.h>

int main(int argc, char const *argv[]) {
	char buffer[] = "TEST";
	int fd = com_tcp_connect(argv[1], argv[2]);
	com_write(fd, buffer, sizeof(buffer), -1);
	com_read(fd, buffer, sizeof(buffer), -1);
	write(0, buffer, 7);
	com_close(fd);
	return 0;
}
