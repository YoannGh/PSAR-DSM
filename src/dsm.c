#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "dsm.h"
#include "dsm_memory.h"
#include "dsm_socket.h"

static void dsm_master_init(dsm_t *dsm, char *host_master, int port_master, size_t page_count)
{
	long pagesize;

	if(page_count <= 0) {
		error("page_count must be greater than 0\n");
	}

	pagesize = sysconf(_SC_PAGE_SIZE);
 	if (pagesize == -1) {
		error("sysconf invalid PAGE_SIZE\n");
 	}

	dsm->mem = (dsm_mem_t *) malloc(sizeof(dsm_mem_t));
	if (dsm->mem == NULL) {
		error("Could not allocate memory (malloc)\n");
	}

	dsm->master = (dsm_master_t *) malloc(sizeof(dsm_master_t));
	if (dsm->master == NULL) {
		error("Could not allocate memory (malloc)\n");
	}

	dsm->master->host = malloc(strlen(host_master)+1);
	if (dsm->master->host == NULL) {
		error("Could not allocate memory (malloc)\n");
	}

	dsm->is_master = 1;
	dsm_memory_init(dsm->mem, pagesize, page_count, dsm->is_master);
	memcpy(dsm->master->host, host_master, strlen(host_master));
	dsm->master->host[strlen(host_master)] = '\0';
	dsm->master->port = port_master;

	if (pthread_create(&dsm->listener_daemon, NULL, &dsm_daemon_msg_listener, (void *) dsm) != 0) {
    	error("pthread_create listener_daemon\n");
	}
}

static void dsm_init(dsm_t *dsm, char *)

static  void dsm_master_destroy(dsm_t *dsm) 
{

}

void *InitMaster(int port, size_t page_count) 
{
	dsm_t *dsm;

	dsm = (dsm_t *) malloc(sizeof(dsm_t));
}

void *InitSlave(char *HostMaster, int port)
{
	return NULL;
}