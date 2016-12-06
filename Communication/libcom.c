#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "libcom.h"


#define LIBCOM_LISTEN_BACKLOG 12


/* Functions to create a non blocking udp/tcp socket and connect it to host@service */
static int com_connect(const char *host, const char *service,
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
	if (status == -1) { perror("com_connect.fcntl"); com_close(fd); return -1; }
	status = fcntl(fd, F_SETFL, status | O_NONBLOCK);
	if (status == -1) { perror("com_connect.fcntl"); com_close(fd); return -1; }
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


/* Functions to bind udp/tcp sockets to service */
static int com_bind(const char *service,
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
	status = getaddrinfo(NULL, service, &hints, &result);
	if (status != 0) {
		fprintf(stderr, "com_bind.getaddrinfo: %s\n", gai_strerror(status));
		return -1;
	}
	/* Go through the linked list and try to connect */
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (-1 == fd) continue;
		if (-1 != bind(fd, rp->ai_addr, rp->ai_addrlen)) break;
		close(fd);
	}
	freeaddrinfo(result);
	/* If we could not connect */
	if (rp == NULL) {
		fprintf(stderr, "com_bind: unable to connect\n");
		return -1;
	}
	/* Set file descriptor to non blocking */
	status = fcntl(fd, F_GETFL, 0);
	if (status == -1) { perror("com_connect.fcntl"); com_close(fd); return -1; }
	status = fcntl(fd, F_SETFL, status | O_NONBLOCK);
	if (status == -1) { perror("com_connect.fcntl"); com_close(fd); return -1; }
	/* Return the connected file descriptor */
	return fd;
}

int com_tcp_bind(const char *service)
{
	int fd = com_bind(service, AI_PASSIVE, AF_UNSPEC, SOCK_STREAM, 0);
	if (fd == -1) return -1;
	/* Set SO_REUSEADDR option */
	int opt = 1;
	if (-1 == setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
		perror("com_tcp_bind.setsockopt"); com_close(fd); return -1;
	}
	/* Listen for new connections */
	if (-1 == listen(fd, LIBCOM_LISTEN_BACKLOG)) {
		perror("com_tcp_bind.listen"); com_close(fd); return -1;
	}
	return fd;
}

int com_udp_bind(const char *service)
{
	return com_bind(service, 0, AF_UNSPEC, SOCK_DGRAM, 0);
}


int com_get_info(int fd, char *host, size_t hostlen, char *serv, size_t servlen)
{
	struct sockaddr_storage remote;
	socklen_t _len = sizeof(remote);
	if (-1 == getpeername(fd, (struct sockaddr *)&remote, &_len)) {
		perror("com_get_host.getpeername");
		return -1;
	}
	int status = getnameinfo((struct sockaddr *)&remote, _len, host, hostlen,
										serv, servlen, 0);
	if (status != 0) {
		fprintf(stderr, "com_get_host.getnameinfo: %s\n", gai_strerror(status));
		return -1;
	}
	return 0;
}

void com_close(int fd)
{
	shutdown(fd, SHUT_RDWR);
	close(fd);
}


/*	Read and write functions
	These functions try to read from or write to the file descriptor exactly
	count bytes and store them in the buffer pointed by buf. If timeout
	(milliseconds) is reached before the fd was available, the function returns
	and gives the amount of bytes left to read or write. If timeout is set to 0,
	the function immediately tries to operate on the fd. If it is set to -1, the
	function will not timeout and will only return if a signal is caught or if
	the underlying socket is closed. */
ssize_t com_read(int fd, void *buf, size_t count, int timeout)
{
#ifdef DEBUG
	size_t _count = count;
#endif
	struct pollfd _fd = { .fd = fd, .events = POLLIN };
	ssize_t size;
	int poll_result = 1;
	int poll_called = 0;
	do {
		switch (poll_result) {
		case -1:
			perror("com_read.poll");
			return -1;
		case 0:
			fprintf(stderr, "com_read.poll: %d timeout (%d ms)\n", fd, timeout);
			goto exit;
		default:
			if (!poll_called || _fd.revents & POLLIN) {
				size = read(fd, buf, count);
				if (size != -1) {
					buf += size;
					count -= size;
					poll_called = 0;
				} else if (errno == EAGAIN) {
					poll_result = poll(&_fd, 1, timeout);
					poll_called = 1;
				} else {
					perror("com_read.read");
					goto exit;
				}
			}
		}
	} while (count);
exit:
#ifdef DEBUG
	printf("RD %d: %zd/%zdB\n", fd, _count - count, _count);
#endif
	return count;
}


ssize_t com_write(int fd, const void *buf, size_t count, int timeout)
{
#ifdef DEBUG
	size_t _count = count;
#endif
	struct pollfd _fd = { .fd = fd, .events = POLLOUT };
	ssize_t size;
	int poll_result = 1;
	int poll_called = 0;
	do {
		switch (poll_result) {
		case -1:
			perror("com_write.poll");
			return -1;
		case 0:
			fprintf(stderr, "com_write.poll: %d timeout (%d ms)\n", fd, timeout);
			goto exit;
		default:
			if (!poll_called || _fd.revents & POLLOUT) {
				size = write(fd, buf, count);
				if (size != -1) {
					buf += size;
					count -= size;
					poll_called = 0;
				} else if (errno == EAGAIN) {
					poll_result = poll(&_fd, 1, timeout);
					poll_called = 1;
				} else {
					perror("com_write.write");
					goto exit;
				}
			}
		}
	} while (count);
exit:
#ifdef DEBUG
	printf("WR %d: %zd/%zdB\n", fd, _count - count, _count);
#endif
	return count;
}
