#ifndef __LIBCOM_H__
#define __LIBCOM_H__

int com_connect_stream(const char *host, const char *service);
int com_connect_dgram(const char *host, const char *service);
int com_bind_stream(const char *service);
int com_bind_dgram(const char *service);
int com_get_info(int fd, char *host, size_t hostlen, char *serv, size_t servlen);
void com_close(int fd);
ssize_t com_read(int fd, void *buf, size_t count, int timeout);
ssize_t com_read_fixed(int fd, void *buf, size_t count, int timeout);
ssize_t com_write(int fd, const void *buf, size_t count, int timeout);

#endif
