#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>

#include <libcom.h>

int main(int argc, char const *argv[]) {
	int fd_bound = com_bind_stream(argv[1]);
	if (fd_bound == -1) return -1;

	printf("Waiting for a client to connect...\n");
	struct pollfd pollfd_bound = { .fd = fd_bound, .events = POLLIN };
	if (poll(&pollfd_bound, 1, -1) <= 0) {
		perror("poll");
		return -1;
	}

	if (!pollfd_bound.revents & POLLIN) return -1;
	int fd_client = accept(fd_bound, NULL, NULL);
	if (fd_client == -1) return -1;
	int status = fcntl(fd_client, F_GETFL, 0);
	if (status == -1) { perror("com_connect.fcntl"); com_close(fd_client); return -1; }
	status = fcntl(fd_client, F_SETFL, status | O_NONBLOCK);
	if (status == -1) { perror("com_connect.fcntl"); com_close(fd_client); return -1; }
	printf("Client connected!\n");
	char host[20], serv[20];
	com_get_info(fd_client, host, sizeof(host), serv, sizeof(serv));
	printf("%d host %s serv %s\n", fd_client, host, serv);

	size_t size = 50 * 1024 * 1024 * sizeof(char);
	unsigned char *buffer = malloc(size);
	size_t _size = com_read_stream(fd_client, buffer, size, -1);
	printf("Read result: %zu\n", _size);

	com_close(fd_bound);
	com_close(fd_client);

	return 0;
}
