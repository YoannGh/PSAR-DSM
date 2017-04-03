#ifndef DSM_SOCKET_H
#define DSM_SOCKET_H

#include <stdint.h>

int dsm_socket_bind_listen(uint32_t port, uint32_t backlog);

int dsm_socket_connect(const char *host, uint32_t port);

int dsm_socket_shutdown(int sockfd, int how);

int dsm_socket_close(int sockfd);

#endif