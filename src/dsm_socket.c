#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "dsm_socket.h"
#include "dsm.h"
#include "dsm_protocol.h"
#include "dsm_util.h"

extern dsm_t *dsm_g;

/**
 * Binds and listens on `port` on all interfaces on the local machine
 * allowing `backlog` clients to be queued
 *
 * @param port the port to use for the server
 * @param backlog the limit of queued connections
 *
 * @return the server socket descriptor
 */
int dsm_socket_bind_listen(int port, int backlog)
{
	struct sockaddr_in sin;
	int sockfd;
	int so_reuseaddr = 1;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		error("socket creation\n");
	}

	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(port);
	sin.sin_family = AF_INET;

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr, sizeof(so_reuseaddr)) < 0) {
		error("socket setsockopt\n");
	}

	if (bind(sockfd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		error("socket bind\n");
	}

	if (listen(sockfd, backlog) < 0) {
		error("socket listen\n");
	}

	return sockfd;
}

/**
 * Connects to `server` and `port` using `ai_family` IP protocol version
 *
 * @param host the hostname of the server to connect to
 * @param port the port to connect on the server machine
 *
 * @return the remote socket descriptor
 */
int dsm_socket_connect(const char *host, int port)
{
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int sockfd;
	char service[16];

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;

	snprintf(service, sizeof(service), "%d", port);

	if (getaddrinfo(host, service, &hints, &result) != 0) {
		error("socket getaddrinfo hostname\n");
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sockfd == -1)
			continue;

		if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1)
			break;

		close(sockfd);
	}

	if (rp == NULL) {
		error("Unable to resolve hostname\n");
	}

	freeaddrinfo(result);

	return sockfd;
}

/**
* \fn int dsm_send(int sockfd, void *buffer, int size)
* \brief send data over network
* \param sockfd socket descriptor to send data
* \param buffer the data to be sent
* \param size the size of the data to send
**/

int dsm_send(int sockfd, void *buffer, int size)
{
	ssize_t bytessent;
	uint32_t msg_size;
	
	msg_size = htonl((uint32_t) size);

	bytessent = send(sockfd, &msg_size, sizeof(uint32_t), 0);
	if(bytessent != sizeof(uint32_t)) {
		error("Send msg size\n");
	}

	bytessent = send(sockfd, buffer, size, 0);
	if(bytessent < 0) {
		error("dsm_send\n");
	} else if(bytessent == 0) {
		debug("send 0 byte, node disconnected\n");
		return -1;
	}

	return 0;
}

/**
* \fn int dsm_receive(int sockfd, void **buffer)
* \brief send data over network
* \param sockfd socket descriptor to receive data
* \param buffer the data to be received
**/

int dsm_receive(int sockfd, void *buffer)
{
	ssize_t bytesrecv;
	uint32_t msg_size;

	bytesrecv = recv(sockfd, &msg_size, sizeof(uint32_t), MSG_WAITALL);
	if(bytesrecv < 0) {
		error("dsm_receive\n");
	} else if(bytesrecv == 0) {
		return -1;
	} else if(bytesrecv != sizeof(uint32_t)) {
		return -1;
	}

	msg_size = ntohl(msg_size);

	if((dsm_g->mem == NULL) && (msg_size > MIN_BUFFERSIZE)) {
		error("Received malformed message too big, could cause buffer overflow\n");
	} 
	else if((dsm_g->mem != NULL) && (msg_size > dsm_g->mem->pagesize + MIN_BUFFERSIZE)) {
		error("Received malformed message too big, could cause buffer overflow\n");
	}

	bytesrecv = recv(sockfd, buffer, msg_size, MSG_WAITALL);
	if(bytesrecv < 0) {
		error("dsm_receive\n");
	} else if(bytesrecv == 0) {
		return -1;
	} else if(bytesrecv != msg_size) {
		return -1;
	}

	return 0;
}

/**
* \fn static int msg_listener_start(dsm_t *dsm)
* \brief function to lisen to messages through network
* \param dsm structure that contains important information
**/

static int msg_listener_start(dsm_t *dsm)
{
	int sock, server_sockfd;
	fd_set active_fd_set, read_fd_set;
	int i;
	struct sockaddr_in clientname;
	socklen_t fromlen;
	dsm_message_t *msg;

	/* Retrieve the already initialized socket */
	sock = dsm->master->sockfd;

	/* Initialize the set of active sockets. */
	FD_ZERO (&active_fd_set);
	FD_SET (sock, &active_fd_set);
	if(dsm->is_master) {
		server_sockfd = dsm->master->server_sockfd;
		FD_SET (server_sockfd, &active_fd_set);
	}

	while (1)
	{
		FD_CLR (dsm->master->sockfd, &active_fd_set);
		FD_SET (dsm->master->sockfd, &active_fd_set);
		
		/* Block until input arrives on one or more active sockets. */
		read_fd_set = active_fd_set;
		if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
			error("select\n");
		}

		/* Service all the sockets with input pending. */
		for (i = 0; i < FD_SETSIZE; ++i) {
			if (FD_ISSET (i, &read_fd_set)) {
				if (dsm->is_master && i == server_sockfd) {
					/* Connection request on listening socket. */
					int new;
					fromlen = sizeof (clientname);
					new = accept(server_sockfd, (struct sockaddr *) &clientname, &fromlen);
					if (new < 0) {
						error("accept\n");
					}

					if (pthread_mutex_lock(&dsm->mutex_client_count) < 0) {
						error("lock mutex_client_count");
					}
					dsm->client_count++;
					if (pthread_mutex_unlock(&dsm->mutex_client_count) < 0) {
						error("unlock mutex_client_count");
					}


					log("Master: connect from host %s, port %hu\n",
					    inet_ntoa (clientname.sin_addr),
					    ntohs (clientname.sin_port));
					FD_SET (new, &active_fd_set);
				}
				else {
					msg = (dsm_message_t *) malloc(sizeof(dsm_message_t));
					if (msg == NULL) {
						error("Could not allocate memory (malloc)\n");
					}
					if(dsm_receive_msg(i, msg) < 0) {
						dsm_socket_close(i);
						free(msg);
						FD_CLR (i, &active_fd_set);
					} else {
						msg->from_sockfd = i;
						dsm_dispatch_message(msg);
						free(msg);
					}
				}
			}
		}
	}
}

/**
* \fn void* dsm_daemon_msg_listener(void *arg)
* \brief function launched in a new thread, starting listener
**/

void* dsm_daemon_msg_listener(void *arg)
{
	dsm_t *dsm = (dsm_t *) arg;
	if(msg_listener_start(dsm) < 0) {
		error("thread msg_listener\n");
	}
	
	return NULL;
}

/**
 * Shut downs the `sockfd` connection, using `how` parameter
 * to behave the same way described in the shutdown(2) man page
 *
 * @param sockfd the socket to be shut down
 * @param how shutdown receptions(SHUT_RD), emissions(SHUT_WR) or
 		  both (SHUT_RDWR).
 *
 * @return 0 on success, else -1
 */
int dsm_socket_shutdown(int sockfd, int how)
{
	return shutdown(sockfd, how);
}

/**
 * Closes the `sockfd` file descriptor
 *
 * @param sockfd the socket descriptor to be closed
 *
 * @return 0 on success, else -1
 */
int dsm_socket_close(int sockfd)
{
	return close(sockfd);
}