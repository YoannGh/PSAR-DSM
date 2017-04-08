#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "dsm_socket.h"
#include "dsm.h"
#include "dsm_protocol.h"
#include "util.h"

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

int dsm_send(int sockfd, void *buffer, int size)
{
	ssize_t bytessent;
	
	bytessent = send(sockfd, buffer, size, 0);
	if(bytessent < 0) {
		error("dsm_send\n");
	} else if(bytessent == 0) {
		debug("dsm_send 0 byte, node disconnected?\n");
		return -1;
	}

	return 0;
}

int dsm_receive(int sockfd, void **buffer, int size)
{
	ssize_t bytesrecv;

	bytesrecv = recv(sockfd, *buffer, size, 0);
	if(bytesrecv < 0) {
		error("dsm_receive\n");
	} else if(bytesrecv == 0) {
		debug("dsm_receive 0 byte, node disconnected?\n");
		return -1;
	}

	return 0;
}

static int msg_listener_start(dsm_t *dsm)
{
	int sock;
	fd_set active_fd_set, read_fd_set;
	int i;
	struct sockaddr_in clientname;
	socklen_t fromlen;
	dsm_message_t *msg;

	/* Create the socket and set it up to accept connections. */
	if (dsm->is_master) {
		sock = dsm_socket_bind_listen(dsm->master.port, MAX_NODES);
	} else {
		sock = dsm_socket_connect(dsm->master.host, dsm->master.port);
		dsm->master.sockfd = sock;
	}

	/* Initialize the set of active sockets. */
	FD_ZERO (&active_fd_set);
	FD_SET (sock, &active_fd_set);

	while (1)
	{
		/* Block until input arrives on one or more active sockets. */
		read_fd_set = active_fd_set;
		if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
			error("select\n");
		}

		/* Service all the sockets with input pending. */
		for (i = 0; i < FD_SETSIZE; ++i) {
			if (FD_ISSET (i, &read_fd_set)) {
				if (dsm->is_master && i == sock) {
					/* Connection request on original socket. */
					int new;
					fromlen = sizeof (clientname);
					new = accept(sock, (struct sockaddr *) &clientname, &fromlen);
					if (new < 0) {
						error("accept\n");
					}
					log("Master: connect from host %s, port %hu\n",
					    inet_ntoa (clientname.sin_addr),
					    ntohs (clientname.sin_port));
					FD_SET (new, &active_fd_set);
				}
				else {
					msg = malloc(sizeof(dsm_message_t));
					if(dsm_receive_msg(i, msg) < 0) {
						dsm_socket_close(i);
						free(msg);
						FD_CLR (i, &active_fd_set);
					} else {
						/* Messages must be free'd by their handlers */
						msg->from_sockfd = i;
						dsm_dispatch_message(msg);
					}
				}
			}
		}
	}
}

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