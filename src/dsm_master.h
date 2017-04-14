#ifndef DSM_MASTER_H
#define DSM_MASTER_H

#define MAX_NODES 512

typedef struct dsm_master_s
{
	char* host;
	int port;
	/* Following fields are used by non master nodes only */
	int sockfd;
} dsm_master_t;

int dsm_master_init(dsm_master_t *master, char *host_master, int port_master, unsigned short is_master);

void dsm_master_destroy(dsm_master_t *master);

#endif