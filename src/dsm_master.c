#include <string.h>

#include "dsm_master.h"
#include "dsm_socket.h"
#include "dsm_util.h"

int dsm_master_init(dsm_master_t *master, char *host_master, int port_master, unsigned short is_master)
{
	char* host_local = "0.0.0.0";
	size_t len;
		
	if(host_master == NULL || is_master == 0) {
		len = strlen(host_local);
		master->host = malloc(len + 1);

		if (master->host == NULL) {
			error("Could not allocate memory (malloc)\n");
		}

		memcpy(master->host, host_master, len);
		master->host[len] = '\0';

	} else {
		len = strlen(host_master);
		master->host = malloc(len + 1);

		if (master->host == NULL) {
			error("Could not allocate memory (malloc)\n");
		}

		memcpy(master->host, host_master, len);
		master->host[len] = '\0';
	}

	master->port = port_master;

	if (is_master) {
		master->sockfd = dsm_socket_bind_listen(master->port, MAX_NODES);
	} else {
		master->sockfd = dsm_socket_connect(master->host, master->port);
	}
	
	return master->sockfd;
}

void dsm_master_destroy(dsm_master_t *master)
{
	dsm_socket_close(master->sockfd);
	free(master->host);
}