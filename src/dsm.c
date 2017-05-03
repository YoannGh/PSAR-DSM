#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <signal.h>

#include "dsm.h"
#include "dsm_core.h"
#include "dsm_memory.h"
#include "dsm_master.h"
#include "dsm_protocol.h"
#include "dsm_socket.h"
#include "list.h"
#include "dsm_util.h"

dsm_t *dsm_g;

/**
 * \fn static void dsm_m_init(dsm_t *dsm, int port_master, size_t page_count)
 * \brief Initialisation of main structure dsm_t for a Master
 * \param dsm the structure dsm_t that will be initialised
 * \param port_master the port of the master
 * \param page_count number of page to be allocated in local memory
 */

static void dsm_m_init(dsm_t *dsm, int port_master, size_t page_count)
{
	long pagesize;

	if (page_count <= 0) {
		error("page_count must be greater than 0\n");
	}

	pagesize = sysconf(_SC_PAGE_SIZE);
	if (pagesize == -1) {
		error("sysconf invalid PAGE_SIZE\n");
	}

	dsm->mem = (dsm_memory_t *) malloc(sizeof(dsm_memory_t));
	if (dsm->mem == NULL) {
		error("Could not allocate memory (malloc)\n");
	}

	dsm->master = (dsm_master_t *) malloc(sizeof(dsm_master_t));
	if (dsm->master == NULL) {
		error("Could not allocate memory (malloc)\n");
	}

	dsm->sync_barrier_waiters = (list_t *) malloc(sizeof(list_t));
	if (dsm->sync_barrier_waiters == NULL) {
		error("Could not allocate memory (malloc)\n");
	}
	list_init(dsm->sync_barrier_waiters, sizeof(int), slave_equals, NULL);

	dsm->is_master = 1;

	dsm_memory_init(dsm->mem, pagesize, page_count, dsm->is_master);

	if (dsm_master_init(dsm->master, NULL, port_master, 1) < 0) {
		error("dsm_master_init\n");
	}

	pthread_mutex_init(&dsm->mutex_client_count, NULL);
	pthread_cond_init(&dsm->cond_master_end, NULL);
	dsm->client_count = 0;

	pthread_mutex_init(&dsm->mutex_sync_barrier, NULL);
	pthread_cond_init(&dsm->cond_sync_barrier, NULL);

	if (pthread_create(&dsm->listener_daemon, NULL, &dsm_daemon_msg_listener, (void *) dsm) != 0) {
		error("pthread_create listener_daemon\n");
	}

	dsm->master->sockfd = dsm_socket_connect(dsm->master->host, dsm->master->port);

	for(unsigned int i = 0; i < page_count; i++) {
		dsm->mem->pages[i].write_owner = dsm->master->sockfd;
	}

}

/**
 * \fn static void dsm_n_init(dsm_t *dsm, char *host_master, int port_master)
 * 	\brief Initialisation of main structure dsm_t for a Slave
 * \param dsm the structure dsm_t that will be initialised
 * \param host_master the inet address of master
 * \param port_master the port of the master
 */

static void dsm_n_init(dsm_t *dsm, char *host_master, int port_master)
{
	dsm_message_t msg_connect;
	dsm_message_t msg_connect_ack;
	long bitness;
	long pagesize;

	bitness = sizeof(void*);

	pagesize = sysconf(_SC_PAGE_SIZE);
	if (pagesize == -1) {
		error("sysconf invalid PAGE_SIZE\n");
	}

	dsm->master = (dsm_master_t *) malloc(sizeof(dsm_master_t));
	if (dsm->master == NULL) {
		error("Could not allocate memory (malloc)\n");
	}

	dsm->is_master = 0;

	if (dsm_master_init(dsm->master, host_master, port_master, 0) < 0) {
		error("dsm_master_init\n");
	}

	msg_connect_args_t ca = {
		.bitness = bitness,
		.pagesize = pagesize
	};
	msg_connect.type = CONNECT;
	msg_connect.connect_args = ca;

	if (dsm_send_msg(dsm->master->sockfd, &msg_connect) < 0) {
		error("Send CONNECT\n");
	}
	if (dsm_receive_msg(dsm->master->sockfd, &msg_connect_ack) < 0) {
		error("Receive CONNECT_ACK\n");
	}
	else {
		if (msg_connect_ack.type != CONNECT_ACK) {
			log("Didn't receive CONNECT_ACK after CONNECT, this should not happen\n");
		}
		else {
			if (!msg_connect_ack.connect_ack_args.bitness_ok) {
				error("Could not join distributed shared memory, this node and the master node don't share the same bitness\n");
			}
			else if (!msg_connect_ack.connect_ack_args.pagesize_ok) {
				error("Could not join distributed shared memory, this node and the master node don't share the same pagesize\n");
			}
			else if (msg_connect_ack.connect_ack_args.page_count <= 0) {
				error("Received negative or 0 page_count from master node\n");
			}

			debug("Joined distributed shared memory !\n");
		}
	}

	dsm->mem = (dsm_memory_t *) malloc(sizeof(dsm_memory_t));
	if (dsm->mem == NULL) {
		error("Could not allocate memory (malloc)\n");
	}

	dsm_memory_init(dsm->mem, pagesize, msg_connect_ack.connect_ack_args.page_count, dsm->is_master);

	pthread_mutex_init(&dsm->mutex_sync_barrier, NULL);
	pthread_cond_init(&dsm->cond_sync_barrier, NULL);

	if (pthread_create(&dsm->listener_daemon, NULL, &dsm_daemon_msg_listener, (void *) dsm) != 0) {
		error("pthread_create listener_daemon\n");
	}
}

/**
 * \fn static void dsm_destroy(dsm_t *dsm)
 * \brief destroy a dsm by calling sub destroying functions for both memory and and master substructure and free them
 * \param dsm the dsm_t structure to be destroyed
 **/

static void dsm_destroy(dsm_t *dsm)
{
	if(pthread_cancel(dsm->listener_daemon) < 0) {
		error("error terminating daemon thread\n");
	}

	dsm_master_destroy(dsm->master);
	dsm_memory_destroy(dsm->mem);
	free(dsm->mem);
	free(dsm->master);

	pthread_mutex_destroy(&dsm->mutex_sync_barrier);
	pthread_cond_destroy(&dsm->cond_sync_barrier);

	if(dsm->is_master) {
		list_destroy(dsm->sync_barrier_waiters);
		free(dsm->sync_barrier_waiters);
		pthread_mutex_destroy(&dsm->mutex_client_count);
		pthread_cond_destroy(&dsm->cond_master_end);
	}
}

/* LIBRARY FUNCTIONS */

/**
 * \fn void *InitMaster(int port, size_t page_count)
 * \brief allocate memory for a Master and call init on it
 * \param port_master the port of the master
 * \param page_count number of page to be allocated in local memory
 * \return the base address of memory
 **/

void *InitMaster(int port, size_t page_count)
{
	if(dsm_g == NULL) {
		dsm_g = (dsm_t *) malloc(sizeof(dsm_t));
		if (dsm_g == NULL) {
			error("Could not allocate memory (malloc)\n");
		}
		dsm_m_init(dsm_g, port, page_count);
	} else {
		log("Warning, you tried to init master while it's already been done\n");
	}
	
	return dsm_g->mem->base_addr;
}

/**
* \fn void *InitSlave(char *HostMaster, int port)
* \brief allocate memory for a Slave and call init on it
* \param host_master the inet address of master
* \param port_master the port of the master
* \return the base address of memory
**/

void *InitSlave(char *HostMaster, int port)
{
	if(dsm_g == NULL) {	
		dsm_g = (dsm_t *) malloc(sizeof(dsm_t));
		if (dsm_g == NULL) {
			error("Could not allocate memory (malloc)\n");
		}
		dsm_n_init(dsm_g, HostMaster, port);
	} else {
		log("Warning, you tried to init slave while it's already been done\n");
	}
	
	return dsm_g->mem->base_addr;
}


/**
* \fn void lock_read(void *addr)
* \brief request for a read access to a page
* \param *addr the adress you want to access
**/
void lock_read(void *addr)
{
	dsm_page_t *page;
	page = get_page_from_addr(addr);
	lock_page(page, PROT_READ);
}

/**
* \fn void lock_write(void *addr)
* \brief request for a write access to a page
* \param *addr the adress you want to access
**/
void lock_write(void *addr)
{
	dsm_page_t *page;	
	page = get_page_from_addr(addr);
	lock_page(page, PROT_READ|PROT_WRITE);
}

/**
* \fn void unlock_read(void *addr)
* \brief release lock on a page after a read access
* \param *addr an adress of the page you want to release
**/
void unlock_read(void *addr)
{
	dsm_page_t *page;
	page = get_page_from_addr(addr);

	if (pthread_mutex_unlock(&page->mutex_page) < 0) {
		error("unlock mutex_page read");
	}
}

/**
* \fn void unlock_write(void *addr)
* \brief send the page to the master and release the lock after a write access 
* \param *addr an adress of the page you want to release
**/
void unlock_write(void *addr)
{
	dsm_page_t *page;
	page = get_page_from_addr(addr);

	dsm_page_request_t req;
	req.rights = page->protection;
	req.sockfd = dsm_g->master->sockfd;

	satisfy_request(page, &req);
	giveup_localpage(page, dsm_g->master->sockfd);

	if (pthread_mutex_unlock(&page->mutex_page) < 0) {
		error("unlock mutex_page write");
	}
}

/**
* \fn void sync_barrier(int slave_to_wait)
* \brief barrier waiting for enough sites to call it
* \param slave_to_wait the numbrer of slave needed to open the barrier
**/
void sync_barrier(int slave_to_wait)
{
	wait_barrier(slave_to_wait);
}

/**
* \fn void QuitDSM(void)
* \brief destroy and free the main dsm_t structure
**/

void QuitDSM(void)
{
	terminate();
	dsm_destroy(dsm_g);
	free(dsm_g);
	debug("Left Distributed Shared Memory !\n");
}
