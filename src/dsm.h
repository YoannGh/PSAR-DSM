#ifndef DSM_H
#define DSM_H

#include "dsm_memory.h"

typedef struct dsm_master_s
{
	char* host;
	int port;
	/* Following fields are used by non master nodes only */
	int sockfd;
} dsm_master_t;

typedef struct dsm_s
{
	dsm_memory_t *mem;
	pthread_t listener_daemon;
	unsigned short is_master;
	dsm_master_t *master;
} dsm_t;

void *InitMaster(int port, size_t page_count);

void *InitSlave(char *HostMaster, int port);

#endif