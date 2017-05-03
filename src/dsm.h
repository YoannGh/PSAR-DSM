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
	pthread_mutex_t mutex_sync_barrier; /*!< mutex to avoid conflicts between daemon and main thread*/
	pthread_cond_t cond_sync_barrier; /*!< condition relative to previous mutex*/
	/* Following fields are only used by master node */
	list_t *sync_barrier_waiters;
	pthread_mutex_t mutex_client_count; /*!< mutex to avoid conflicts between daemon and main thread*/
	pthread_cond_t cond_master_end; /*!< condition relative to previous mutex*/
	int client_count;
} dsm_t;

void *InitMaster(int port, size_t page_count);

void *InitSlave(char *HostMaster, int port);

void lock_read(void *addr);

void lock_write(void *addr);

void unlock_read(void *addr);

void unlock_write(void *addr);

void sync_barrier(int slave_to_wait);

void QuitDSM(void);

#endif