#ifndef DSM_SOCKET_H
#define DSM_SOCKET_H

int dsm_socket_bind_listen(int port, int backlog);

int dsm_socket_connect(const char *host, int port);

int dsm_socket_shutdown(int sockfd, int how);

int dsm_socket_close(int sockfd);

#endif