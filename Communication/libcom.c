#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>


static inline int com_connect(const char *host, const char *service,
										int family, int socktype)
{
	struct addrinfo hints, *result, *rp;
	int status, fd;

	/* Obtain addresses matching host/service */
	memset (&hints, 0, sizeof(hints));
	hints.ai_family = family;
	hints.ai_socktype = socktype;
	/* Get address info */
	status = getaddrinfo(host, service, &hints, &result);
	if (status != 0) {
		fprintf(stderr, "com_connect.getaddrinfo: %s\n", gai_strerror(status));
		return -1;
	}
	/* Go through the linked list and try to connect */
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (-1 == fd) continue;
		if (-1 != connect(fd, rp->ai_addr, rp->ai_addrlen)) break;
		close(fd);
	}
	freeaddrinfo(result);
	/* If we could not connect */
	if (rp == NULL) {
		fprintf(stderr, "com_connect: unable to connect\n");
		return -1;
	}
	/* Set file descriptor to non blocking */
	status = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, status | O_NONBLOCK);
	/* Return the connected file descriptor */
	return fd;
}


int com_tcp_connect(const char *host, const char *service)
{
	return com_connect(host, service, AF_UNSPEC, SOCK_STREAM);
}


int com_udp_connect(const char *host, const char *service)
{
	return com_connect(host, service, AF_UNSPEC, SOCK_DGRAM);
}


int com_tcp_bind(const char *service)
{
	return 0;
}

ssize_t com_read(int fd, void *buf, size_t count, int timeout)
{
	struct pollfd _fd = { .fd = fd, .events = POLLIN };
	switch (poll(&_fd, 1, timeout)) {
	case -1:
		fprintf(stderr, "com_read.poll: %s\n", strerror(errno));
		return -1;
	case 0:
		fprintf(stderr, "com_read.poll: %d timeout (%d ms)\n", fd, timeout);
		return -1;
	default:
		if (_fd.revents & POLLIN)
			return read(fd, buf, count);
	}
	return -1;
}


ssize_t com_write(int fd, void *buf, size_t count, int timeout)
{
	struct pollfd _fd = { .fd = fd, .events = POLLOUT };
	size_t written;
	do {
		switch (poll(&_fd, 1, timeout)) {
		case -1:
			fprintf(stderr, "com_write.poll: %s\n", strerror(errno));
			return -1;
		case 0:
			fprintf(stderr, "com_write.poll: %d timeout (%d ms)\n", fd, timeout);
			return -1;
		default:
			if (_fd.revents & POLLOUT) {
				written = write(fd, buf, count);
				buf += written;
				count -= written;
			}
		}
	} while (count);
	return 0;
}
