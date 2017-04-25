#ifndef DSM_H
#define DSM_H

#include <pthread.h>

#include "dsm_memory.h"
#include "dsm_master.h"

typedef struct dsm_s
{
	dsm_memory_t *mem;
	pthread_t listener_daemon;
	unsigned short is_master;
	dsm_master_t *master;
} dsm_t;

void *InitMaster(int port, size_t page_count);

void *InitSlave(char *HostMaster, int port);

void lock_read(void *adr);

void lock_write(void *adr);

void unlock_read(void *adr);

void unlock_write(void *adr);

void QuitDSM(void);

#endif