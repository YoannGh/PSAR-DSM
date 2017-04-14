#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "dsm_socket.h"
#include "dsm_util.h"

int main(void)
{
	struct sockaddr exp;
	int socket_server, socket_client;
	socklen_t fromlen = sizeof exp;

	socket_server = dsm_socket_bind_listen(5555, 256);

	while (1)
	{
		puts("accepting connections...");
		socket_client = accept(socket_server, &exp, &fromlen);
		if (socket_client < 0) {
			error("error accept client\n");
		} else {
			log("New client fd: %d\n", socket_client);
		}
	}

	dsm_socket_close(socket_server);

	return 0;
}