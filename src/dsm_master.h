#ifndef DSM_MASTER_H
#define DSM_MASTER_H

#define MAX_NODES 512

/**
 * \struct dsm_master_s
 * \brief substructure containing information about the master itself or the master related to the current slave

 **/
typedef struct dsm_master_s
{
	char* host; /*!< a string representing the master inet address */
	int port; /*!< port master is listening to*/
	/* Following fields are used by non master nodes only */
	int sockfd; /*!< socket identifer related to the master */
	int server_sockfd; /*!< socket indentifier where master is listening*/
} dsm_master_t;

int dsm_master_init(dsm_master_t *master, char *host_master, int port_master, unsigned short is_master);

void dsm_master_destroy(dsm_master_t *master);

#endif