#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>


#include "dsm.h"
#include "dsm_memory.h"
#include "dsm_master.h"
#include "dsm_protocol.h"
#include "dsm_socket.h"
#include "dsm_util.h"
#include "dsm_core.h"

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

	dsm->is_master = 1;

	dsm_memory_init(dsm->mem, pagesize, page_count, dsm->is_master);

	if (dsm_master_init(dsm->master, NULL, port_master, 1) < 0) {
		error("dsm_master_init\n");
	}

	if (pthread_create(&dsm->listener_daemon, NULL, &dsm_daemon_msg_listener, (void *) dsm) != 0) {
		error("pthread_create listener_daemon\n");
	}

	dsm->master->sockfd = dsm_socket_connect(dsm->master->host, dsm->master->port);
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
	dsm_master_destroy(dsm->master);
	dsm_memory_destroy(dsm->mem);
	/* TODO: Terminate daemon thread */
	free(dsm->mem);
	free(dsm->master);
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
		log("Tried to init master while it's already been done\n");
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
		log("Tried to init master while it's already been done\n");
	}
	
	return dsm_g->mem->base_addr;
}

<<<<<<< HEAD
/**
* \fn void QuitDSM(void)
* \brief destroy and free the main dsm_t structure
**/
=======
void lock_page(dsm_page_t *page, int rights)
{
	dsm_message_t msg_lockpage;
	msg_lockpage.type = LOCKPAGE;

	msg_lockpage_args_t la = {
		.page_id = page->page_id,
		.access_rights = rights 
	};

	msg_lockpage.lockpage_args = la;

	if (dsm_send_msg(dsm_g->master->sockfd, &msg_lockpage) < 0) {
		error("Send LOCK\n");
	}

	while( pthread_cond_wait(&page->cond_uptodate, &page->mutex_page) != 0 );
}

void lock_read(void *adr)
{
	dsm_page_t *page;	
	page = get_page_from_addr(adr);
	lock_page(page, PROT_READ);
}

void lock_write(void *adr)
{
	dsm_page_t *page;	
	page = get_page_from_addr(adr);
	lock_page(page, PROT_WRITE);
}

/*
void unlock_read(void *adr)
{

}
*/

void unlock_write(void *adr)
{
	dsm_page_t *page;	
	page = get_page_from_addr(adr);
	dsm_page_request_t req;
	req.rights = PROT_WRITE;
	req.sockfd = dsm_g->master->sockfd;
	satisfy_request(page, &req);
}
>>>>>>> algo

void QuitDSM(void)
{
	/* TOOD: Send pages owned by this node to master */
	dsm_destroy(dsm_g);
	free(dsm_g);
}