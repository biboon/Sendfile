#ifndef __LIBCOM_H__
#define __LIBCOM_H__

int com_tcp_connect(const char *host, const char *service);
int com_udp_connect(const char *host, const char *service);
int com_tcp_bind(const char *service);
ssize_t com_read(int fd, void *buf, size_t count, int timeout);
ssize_t com_write(int fd, void *buf, size_t count, int timeout);

#endif