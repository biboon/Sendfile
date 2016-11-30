#ifndef __LIBCOM_H__
#define __LIBCOM_H__

int com_tcp_connect(const char *host, const char *service);
int com_udp_connect(const char *host, const char *service);
int com_tcp_bind(const char *service);
int com_udp_bind(const char *service);
int com_get_info(int fd, char *host, size_t hostlen, char *serv, size_t servlen);
void com_close(int fd);
ssize_t com_read(int fd, void *buf, size_t count, int timeout);
ssize_t com_write(int fd, void *buf, size_t count, int timeout);

#endif
