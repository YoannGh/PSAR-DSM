#ifndef DSM_H
#define DSM_H

#include <pthread.h>

#include "dsm_memory.h"
#include "dsm_master.h"

/**
 * \struct dsm_s
 * \brief principal structure of a dsm node containing all its information

 **/

typedef struct dsm_s
{
	dsm_memory_t *mem; /*!<substructure containing memory pages */
	pthread_t listener_daemon; /*!< listener daemon indentifier */
	unsigned short is_master; /*!< flag that differentiate master node and slave node */
	dsm_master_t *master; /*!< substructure containing information about the master itself or the master related to the slave. */
} dsm_t;

void *InitMaster(int port, size_t page_count);

void *InitSlave(char *HostMaster, int port);

void QuitDSM(void);

#endif