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
static int com_connect(const char *host, const char *service, int flags,
	int family, int socktype, int protocol)
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

int com_connect_stream(const char *host, const char *service)
{
	return com_connect(host, service, 0, AF_UNSPEC, SOCK_STREAM, 0);
}

int com_connect_dgram(const char *host, const char *service)
{
	return com_connect(host, service, 0, AF_UNSPEC, SOCK_DGRAM, 0);
}


/* Functions to bind udp/tcp sockets to service */
static int com_bind(const char *service, int flags, int family, int socktype,
	int protocol)
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

int com_bind_stream(const char *service)
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

int com_bind_dgram(const char *service)
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


/* This functions performs a single call to read and saves in buffer pointed by
 * buf at most count bytes. Thus, this function is more suited to be used on
 * SOCK_DGRAM type sockets. However, this should not be a problem if you use it
 * on SOCK_STREAM sockets. */
ssize_t com_read_dgram(int fd, void *buf, size_t count, int timeout)
{
	ssize_t size = read(fd, buf, count);
	if (size != -1) return size;
	if (errno == EAGAIN) {
		struct pollfd _fd = { .fd = fd, .events = POLLIN };
		switch (poll(&_fd, 1, timeout)) {
		case -1:
			perror("com_read_dgram.poll");
			return -1;
		case 0:
			fprintf(stderr, "com_read_dgram.poll: %d timeout (%d ms)\n", fd, timeout);
			return 0;
		default:
			size = read(fd, buf, count);
			if (size != -1) {
#ifdef DEBUG
				printf("RD dgram %d: %zd/%zdB\n", fd, size, count);
#endif
				return size;
			}
			perror("com_read_dgram.read");
			return -1;
		};
	}
	perror("com_read_dgram.read");
	return -1;
}

/* This function reads from the file descriptor fd and saves exactly count bytes
 * in the buffer pointed by buf. Thus, this function is suited to be used on
 * SOCK_STREAM sockets, but definitely NOT on SOCK_DGRAM. */
ssize_t com_read_stream(int fd, void *buf, size_t count, int timeout)
{
	size_t _count = count;
	struct pollfd _fd = { .fd = fd, .events = POLLIN, .revents = POLLIN };
	int poll_result = 1;
	do {
		switch (poll_result) {
		case -1:
			perror("com_read_stream.poll");
			return -1;
		case 0:
			fprintf(stderr, "com_read_stream.poll: %d timeout (%d ms)\n", fd, timeout);
			goto exit;
		default:
			if (_fd.revents & POLLIN) {
				ssize_t size = read(fd, buf, count);
				if (size > 0) {
					buf += size;
					count -= size;
					_fd.revents |= POLLIN;
				} else if (size == -1 && errno == EAGAIN) {
					poll_result = poll(&_fd, 1, timeout);
				} else if (size == 0) {
					goto exit;
				} else {
					perror("com_read_stream.read");
					return -1;
				}
			}
		}
	} while (count);
exit:
#ifdef DEBUG
	printf("RD stream %d: %zd/%zdB\n", fd, _count - count, _count);
#endif
	return _count - count;
}

/* This function writes exactly count bytes to the file descriptor fd. For
 * SOCK_STREAM sockets, there is no problem, however for SOCK_DGRAM, if the
 * size specified in count is too large, there should be an error. The data to
 * write is stored in the buffer pointed by buf. */
ssize_t com_write(int fd, const void *buf, size_t count, int timeout)
{
	size_t _count = count;
	struct pollfd _fd = { .fd = fd, .events = POLLOUT, .revents = POLLOUT };
	int poll_result = 1;
	do {
		switch (poll_result) {
		case -1:
			perror("com_write.poll");
			return -1;
		case 0:
			fprintf(stderr, "com_write.poll: %d timeout (%d ms)\n", fd, timeout);
			goto exit;
		default:
			if (_fd.revents & POLLOUT) {
				ssize_t size = write(fd, buf, count);
				if (size > 0) {
					buf += size;
					count -= size;
					_fd.revents |= POLLOUT;
				} else if (size == -1 && errno == EAGAIN) {
					poll_result = poll(&_fd, 1, timeout);
				} else {
					perror("com_write.write");
					return -1;
				}
			}
		}
	} while (count);
exit:
#ifdef DEBUG
	printf("WR %d: %zd/%zdB\n", fd, _count - count, _count);
#endif
	return _count - count;
}
