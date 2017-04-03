#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "dsm_socket.h"
#include "util.h"

int main(void)
{
	struct sockaddr exp;
	int socket_server;
	socklen_t fromlen = sizeof exp;

	socket_server = dsm_socket_bind_listen(5555, 256);
	
	while(1)
	{
		puts("accepting connections...");
		if(accept(socket_server, &exp, &fromlen) < 0)
			handle_err("error accept client", DSM_EXIT);

		puts("New connection!");
	}
	
	dsm_socket_close(socket_server);

	return 0;
}