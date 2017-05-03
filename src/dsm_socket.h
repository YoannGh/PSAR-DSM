#ifndef DSM_SOCKET_H
#define DSM_SOCKET_H

#define MIN_BUFFERSIZE 512

int dsm_socket_bind_listen(int port, int backlog);

int dsm_socket_connect(const char *host, int port);

int dsm_send(int sockfd, void *buffer, int size);

int dsm_receive(int sockfd, void *buffer);

void* dsm_daemon_msg_listener(void *arg);

int dsm_socket_shutdown(int sockfd, int how);

int dsm_socket_close(int sockfd);

#endif