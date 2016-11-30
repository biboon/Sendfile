#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>

#include <libcom.h>

int main(int argc, char const *argv[]) {
	int fd_bound = com_tcp_bind(argv[1]);
	if (fd_bound == -1) return -1;

	printf("Waiting for a client to connect...\n");
	struct pollfd pollfd_bound = { .fd = fd_bound, .events = POLLIN };
	if (poll(&pollfd_bound, 1, -1) <= 0) {
		fprintf(stderr, "poll %s\n", strerror(errno));
		return -1;
	}

	if (!pollfd_bound.revents & POLLIN) return -1;
	int fd_client = accept(fd_bound, NULL, NULL);
	if (fd_client == -1) return -1;
	printf("Client connected!\n");

	size_t size = 0;
	com_read(fd_client, &size, sizeof(size), -1);
	printf("Read size: %zu\n", size);

	com_close(fd_bound);
	com_close(fd_client);

	return 0;
}
