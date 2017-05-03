#include <string.h>
#include <sys/socket.h>


#include "dsm_master.h"
#include "dsm_socket.h"
#include "dsm_util.h"

/**
 * \fn int dsm_master_init (dsm_master_t *master, char *host_master, int port_master, unsigned short is_master)
 * \brief Initialisation of substructure dedicated to master 
 *
 * \param master the structure that will be initialised
 * \param host_master the address of the master
 * \param port_master the port of the master
 * \param is_master a flag to know if the structure is a master or if is only related to the master
 * \return the socket descriptor
 */

int dsm_master_init(dsm_master_t *master, char *host_master, int port_master, unsigned short is_master)
{
	char* host_local = "127.0.0.1";
	size_t len;
		
	if(host_master == NULL || is_master == 1) {
		len = strlen(host_local);
		master->host = malloc(len + 1);

		if (master->host == NULL) {
			error("Could not allocate memory (malloc)\n");
		}

		memcpy(master->host, host_local, len);
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
		master->server_sockfd = dsm_socket_bind_listen(master->port, MAX_WAITING_NODES);
	} else {
		master->sockfd = dsm_socket_connect(master->host, master->port);
	}
	
	return master->sockfd;
}

/**
 * \fn void dsm_master_destroy(dsm_master_t *master)
 * \brief destroy socket used by the structure and free memory
 * \param master the structure that will be destroyed
 **/

void dsm_master_destroy(dsm_master_t *master)
{
	dsm_socket_shutdown(master->sockfd, SHUT_RD);
	free(master->host);
}