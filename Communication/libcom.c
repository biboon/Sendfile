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
	int flags, int family, int socktype, int protocol)
{
	int status, fd;
	struct addrinfo *result, *rp;
	/* Set hints to pass to getaddrinfo */
	const struct addrinfo hints = {
		.ai_flags = flags,
		.ai_family = family,
		.ai_socktype = socktype,
		.ai_protocol = protocol
	};

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
	return com_connect(host, service, 0, AF_UNSPEC, SOCK_STREAM, 0);
}


int com_udp_connect(const char *host, const char *service)
{
	return com_connect(host, service, 0, AF_UNSPEC, SOCK_DGRAM, 0);
}


int com_tcp_bind(const char *service)
{
	return 0;
}

int com_udp_bind(const char *service)
{
	return 0;
}

void com_close(int fd)
{
	shutdown(fd, SHUT_RDWR);
	close(fd);
}

ssize_t com_read(int fd, void *buf, size_t count, int timeout)
{
	struct pollfd _fd = { .fd = fd, .events = POLLIN };
	ssize_t size;
	do {
		switch (poll(&_fd, 1, timeout)) {
		case -1:
			fprintf(stderr, "com_read.poll: %s\n", strerror(errno));
			return -1;
		case 0:
			fprintf(stderr, "com_read.poll: %d timeout (%d ms)\n", fd, timeout);
			return count;
		default:
			if (_fd.revents & POLLIN) {
				size = read(fd, buf, count);
				if (-1 == size) {
					fprintf(stderr, "com_read.read: %s\n", strerror(errno));
					return count;
				}
				buf += size;
				count -= size;
			}
		}
	} while (count);
	return 0;
}


ssize_t com_write(int fd, void *buf, size_t count, int timeout)
{
	struct pollfd _fd = { .fd = fd, .events = POLLOUT };
	ssize_t size;
	do {
		switch (poll(&_fd, 1, timeout)) {
		case -1:
			fprintf(stderr, "com_write.poll: %s\n", strerror(errno));
			return -1;
		case 0:
			fprintf(stderr, "com_write.poll: %d timeout (%d ms)\n", fd, timeout);
			return count;
		default:
			if (_fd.revents & POLLOUT) {
				size = write(fd, buf, count);
				if (-1 == size) {
					fprintf(stderr, "com_write.write: %s\n", strerror(errno));
					return count;
				}
				buf += size;
				count -= size;
			}
		}
	} while (count);
	return 0;
}
