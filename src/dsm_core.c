#include <sys/mman.h>
#include <string.h>
#include <sys/socket.h>

#include "dsm_core.h"
#include "dsm.h"
#include "dsm_memory.h"
#include "dsm_protocol.h"
#include "dsm_socket.h"
#include "dsm_util.h"
#include "list.h"

extern dsm_t *dsm_g;

static void process_list_requests(dsm_page_t *page);

/**
* \fn int handle_connect_msg(int from, msg_connect_args_t *args)
* \brief processing a connect msg
* \param from the sockfd who sent the data
* \param *args the message structure
* \return the send status 
**/
int handle_connect_msg(int from, msg_connect_args_t *args)
{
	dsm_message_t reply;
	reply.type = CONNECT_ACK;

	unsigned short bitness_ok = 0;
	unsigned short pagesize_ok = 0;

	if (args->bitness == sizeof(void*))
		bitness_ok = 1;
	if (args->pagesize == dsm_g->mem->pagesize)
		pagesize_ok = 1;

	msg_connect_ack_args_t ca = {
		.bitness_ok = bitness_ok,
		.pagesize_ok = pagesize_ok,
		.page_count = dsm_g->mem->page_count
	};

	reply.connect_ack_args = ca;

	return dsm_send_msg(from, &reply);
}

/**
* \fn int handle_lockpage_msg(int from, msg_lockpage_args_t *args)
* \brief processing a lockpage request
* \param from the sockfd who sent the data
* \param *args the message structure
* \return 0
**/
int handle_lockpage_msg(int from, msg_lockpage_args_t *args)
{
	dsm_page_t *page;
	dsm_page_request_t new_req = {
		.sockfd = from,
		.rights = args->access_rights
	};

	if (args->access_rights & PROT_WRITE)
		log("Received LOCKWRITE on page %lu from %d\n", args->page_id, from);
	else if (args->access_rights == PROT_READ)
		log("Received LOCKREAD on page %lu from %d\n", args->page_id, from);
	else
		debug("Received unknown LOCK on page %lu from %d\n", args->page_id, from);

	page = get_page_from_id(args->page_id);

	list_add(page->requests_queue, &new_req);
	process_list_requests(page);

	return 0;
}

/**
* \fn int handle_invalidate_msg(int from, msg_invalidate_args_t *args)
* \brief processing a invalidation request
* \param from the sockfd who sent the data
* \param *args the message structure
* \return the send status 
**/
int handle_invalidate_msg(int from, msg_invalidate_args_t *args)
{
	dsm_page_t *page;
	void* page_base_addr;

	page = get_page_from_id(args->page_id);
	page_base_addr = dsm_g->mem->base_addr + (args->page_id * dsm_g->mem->pagesize);

	log("Received INVALIDATE for page %lu from %d\n", args->page_id, from);
	if (pthread_mutex_lock(&page->mutex_page) < 0) {
		error("lock mutex_page");
	}

	page->uptodate = 0;
	if (mprotect(page_base_addr, dsm_g->mem->pagesize, PROT_NONE) < 0) {
		error("error mprotect\n");
	}
	page->protection = PROT_NONE;

	if (pthread_mutex_unlock(&page->mutex_page) < 0) {
		error("unlock mutex_page");
	}

	//Send INVALIDATE_ACK
	dsm_message_t reply;
	reply.type = INVALIDATE_ACK;

	msg_invalidate_ack_args_t ca = {
		.page_id = args->page_id,
	};

	reply.invalidate_ack_args = ca;

	return dsm_send_msg(from, &reply);
}

/**
* \fn int handle_invalidate_ack_msg(int from, msg_invalidate_ack_args_t *args)
* \brief processing validation ack msg
* \param from the sockfd who sent the data
* \param *args the message structure
* \return 0 
**/
int handle_invalidate_ack_msg(int from, msg_invalidate_ack_args_t *args)
{
	dsm_page_t *page;

	page = get_page_from_id(args->page_id);
	log("Received INVALIDATE_ACK for page %lu from %d\n", args->page_id, from);

	list_remove(page->current_readers_queue, &from);
	process_list_requests(page);

	return 0;
}

/**
* \fn int handle_givepage_msg(int from, msg_givepage_args_t *args)
* \brief processing a page reception and set the rights
* \param from the sockfd who sent the data
* \param *args the message structure
* \return 0
**/
int handle_givepage_msg(int from, msg_givepage_args_t *args)
{
	dsm_page_t *page;
	void* page_base_addr;

	page = get_page_from_id(args->page_id);
	page_base_addr = dsm_g->mem->base_addr + (args->page_id * dsm_g->mem->pagesize);

	debug("Received GIVEPAGE on page %lu from %d with rights %d\n", args->page_id, from, args->access_rights);

	if (pthread_mutex_lock(&page->mutex_page) < 0) {
		error("lock mutex_page");
	}
	if (mprotect(page_base_addr, dsm_g->mem->pagesize, PROT_READ | PROT_WRITE) < 0) {
		error("error mprotect\n");
	}

	memcpy(page_base_addr, args->data, dsm_g->mem->pagesize);

	if (mprotect(page_base_addr, dsm_g->mem->pagesize, args->access_rights) < 0) {
		error("error mprotect\n");
	}

	page->protection = args->access_rights;
	page->write_owner = dsm_g->master->sockfd;
	page->uptodate = 1;

	if (pthread_cond_signal(&page->cond_uptodate) < 0) {
		error("error pthread_cond_signal mutex_page\n");
	}

	if (pthread_mutex_unlock(&page->mutex_page) < 0) {
		error("unlock mutex_page");
	}

	if (dsm_g->is_master) {
		process_list_requests(page);
	}

	return 0;
}

/**
* \fn int handle_sync_barrier_msg(int from, msg_sync_barrier_args_t *args)
* \brief handle the barrier, open it if the msg is from the last slave 
* \param from the sockfd who sent the data
* \param *args the message structure
* \return 0
**/
int handle_sync_barrier_msg(int from, msg_sync_barrier_args_t *args)
{
	listNode_t *waiterNode = dsm_g->sync_barrier_waiters->head;
	int *waiter;
	int waiter_fd = 0;

	dsm_message_t msg_barrier_ack;
	msg_barrier_ack.type = BARRIER_ACK;

	if (dsm_g->is_master) {

		if ((dsm_g->sync_barrier_waiters->length + 1) >= args->slave_to_wait) {
			debug("Broadcasting BARRIER_ACK\n");
			while (waiterNode != NULL) {
				waiter = (int *) waiterNode->data;
				waiter_fd = *waiter;

				if (dsm_send_msg(waiter_fd, &msg_barrier_ack) < 0) {
					log("error sending BARRIER_ACK\n");
				}

				waiterNode = waiterNode->next;
			}
			if (dsm_send_msg(from, &msg_barrier_ack) < 0) {
				log("error sending BARRIER_ACK\n");
			}
			list_destroy(dsm_g->sync_barrier_waiters);
		}
		else {
			list_add(dsm_g->sync_barrier_waiters, &from);
		}
	}

	return 0;
}

/**
* \fn int handle_barrier_ack_msg(int from)
* \brief open berrier for the slave
* \param from the sockfd who sent the data
* \return 0
**/
int handle_barrier_ack_msg(int from)
{
	if (pthread_cond_signal(&dsm_g->cond_sync_barrier) < 0) {
		error("error pthread_cond_signal SYNC_BARRIER\n");
	}
	return 0;
}

/**
* \fn int handle_terminate_msg(int from)
* \brief handle a slave deconnection
* \param from the sockfd who sent the data
* \return 0
**/
int handle_terminate_msg(int from)
{
	if(dsm_g->is_master) {

		if (pthread_mutex_lock(&dsm_g->mutex_client_count) < 0) {
			error("lock mutex_client_count");
		}
		dsm_g->client_count--;

		if(dsm_g->client_count == 0) {
			if (pthread_cond_signal(&dsm_g->cond_master_end) < 0) {
				error("error pthread_cond_signal cond_master_end\n");
			}
		}

		if (pthread_mutex_unlock(&dsm_g->mutex_client_count) < 0) {
			error("unlock mutex_client_count");
		}
	}

	//dsm_socket_shutdown(from, SHUT_RDWR);
	//return dsm_socket_close(from);
	return 0;
}

/**
* \fn void lock_page(dsm_page_t *page, int rights)
* \brief send a request to the master for a page access
* \param *page an adress of the page 
* \param rights the rights requested 
**/
void lock_page(dsm_page_t *page, int rights)
{

	if (pthread_mutex_lock(&page->mutex_page) < 0) {
		error("lock mutex_page");
	}

	if (!page->uptodate) {

		dsm_message_t msg_lockpage;
		msg_lockpage.type = LOCKPAGE;

		msg_lockpage_args_t la = {
			.page_id = page->page_id,
			.access_rights = rights
		};

		msg_lockpage.lockpage_args = la;

		if (dsm_send_msg(dsm_g->master->sockfd, &msg_lockpage) < 0) {
			error("Send LOCKPAGE\n");
		}

		debug("Sent LOCKPAGE to master for page %lu\n", page->page_id);

		while (!page->uptodate) {
			if (pthread_cond_wait(&page->cond_uptodate, &page->mutex_page) < 0) {
				error("error pthread_cond_wait mutex_page\n");
			}
		}
	}
}

/**
* \fn int satisfy_request(dsm_page_t *page, dsm_page_request_t *req)
* \brief send a page to a slave
* \param *page the adress of the page to send 
* \param *req the request (slave ID and rights)
* \return send status 
**/
int satisfy_request(dsm_page_t *page, dsm_page_request_t *req)
{
	dsm_message_t givepage_msg;
	givepage_msg.type = GIVEPAGE;

	msg_givepage_args_t ca = {
		.page_id = page->page_id,
		.access_rights = req->rights,
		.data = dsm_g->mem->base_addr + (page->page_id * dsm_g->mem->pagesize)
	};
	givepage_msg.givepage_args = ca;

	debug("Sending page %lu to %d current rights: %d\n", page->page_id, req->sockfd, page->protection);
	return dsm_send_msg(req->sockfd, &givepage_msg);
}

/**
* \fn void invalidate_readers(dsm_page_t *page)
* \brief send an invalidation request to all current readers of a page
* \param *page the adress of the page to invalidate 
**/
static void invalidate_readers(dsm_page_t *page)
{
	listNode_t *readerNode = page->current_readers_queue->head;
	int *reader;
	int reader_sockfd = 0;

	dsm_message_t msg_invalidate;
	msg_invalidate.type = INVALIDATE;

	msg_invalidate_args_t ia = {
		.page_id = page->page_id,
	};
	msg_invalidate.invalidate_args = ia;

	while (readerNode != NULL) {
		reader = (int *) readerNode->data;
		reader_sockfd = *reader;

		if (dsm_send_msg(reader_sockfd, &msg_invalidate) < 0) {
			log("error sending INVALIDATE\n");
		}

		readerNode = readerNode->next;
	}
}

/**
* \fn void giveup_localpage(dsm_page_t *page, int new_owner)
* \brief update page status and erase rigths on it
* \param *page the adress of the page
* \param new_owner the new page's owner
**/
void giveup_localpage(dsm_page_t *page, int new_owner)
{
	void* page_base_addr;

	page_base_addr = dsm_g->mem->base_addr + (page->page_id * dsm_g->mem->pagesize);
	page->uptodate = 0;
	page->protection = PROT_NONE;
	page->write_owner = new_owner;

	if (mprotect(page_base_addr, dsm_g->mem->pagesize, PROT_NONE) < 0) {
		error("error mprotect\n");
	}
}

/**
* \fn void wait_barrier(int slave_to_wait)
* \brief barrier waiting for enough sites to call it
* \param slave_to_wait the numbrer of slave needed to open the barrier
**/
void wait_barrier(int slave_to_wait)
{
	if (pthread_mutex_lock(&dsm_g->mutex_sync_barrier) < 0) {
		error("lock mutex_sync_barrier");
	}

	dsm_message_t msg_sync_barrier;
	msg_sync_barrier.type = SYNC_BARRIER;

	msg_sync_barrier_args_t sba = {
		.slave_to_wait = slave_to_wait,
	};
	msg_sync_barrier.sync_barrier_args = sba;

	if (dsm_send_msg(dsm_g->master->sockfd, &msg_sync_barrier) < 0) {
		error("error sending SYNC_BARRIER message\n");
	}

	debug("Waiting for BARRIER_ACK message\n");
	if (pthread_cond_wait(&dsm_g->cond_sync_barrier, &dsm_g->mutex_sync_barrier) < 0) {
		error("error pthread_cond_wait SYNC_BARRIER\n");
	}

	if (pthread_mutex_unlock(&dsm_g->mutex_sync_barrier) < 0) {
		error("unlock mutex_sync_barrier");
	}
}

/**
* \fn void process_list_requests(dsm_page_t *page)
* \brief try to process as many request as possible for a page
* \param *page the page to handle
**/
static void process_list_requests(dsm_page_t *page)
{
	listNode_t *reqlist = page->requests_queue->head;
	dsm_page_request_t *req;

	//If their's a write_owner, we must wait for the write_unlock
	if (!page->uptodate)
		return;

	while (reqlist != NULL)
	{
		req = (dsm_page_request_t *) reqlist->data;
		//Case it's a reader, juste have to check the page isn't lock in write.
		//if not, just send the page, update both queues and process new head element
		if (req->rights == PROT_READ)
		{
			if (satisfy_request(page, req) < 0) {
				log("error satisfy_request\n");
			}
			list_add(page->current_readers_queue, &req->sockfd);
			list_remove(page->requests_queue, req);
			reqlist = reqlist->next;
			continue;
		}
		//Case it's a writer, if there is some readers we must ask them to invalidate
		//their pages, and wait for all of them to realease their lock (ack they know the
		//file is dirty)
		else if (req->rights & PROT_WRITE)
		{
			if (!page->invalidate_sent && page->current_readers_queue->length > 0)
			{
				invalidate_readers(page);
				page->invalidate_sent = 1;
			}
			else if (page->current_readers_queue->length == 0)
			{
				page->invalidate_sent = 0;
				if (satisfy_request(page, req) < 0) {
					log("error satisfy_request\n");
				}
				giveup_localpage(page, req->sockfd);
				list_remove(page->requests_queue, req);
				return;
			}
		}
		else
		{
			error("Bad Rights");
			return;
		}
	}
}

/**
* \fn int terminate(void) 
* \brief Properly exiting the DSM 
* \return 0
**/
int terminate(void) 
{
	dsm_page_t *page;
	dsm_message_t msg_terminate;
	msg_terminate.type = TERMINATE;

	dsm_page_request_t req;
	req.rights = PROT_READ|PROT_WRITE;
	req.sockfd = dsm_g->master->sockfd;

	if(!dsm_g->is_master) {
		debug("Leaving system: giving all my write pages to master\n");
		for(unsigned int i = 0; i < dsm_g->mem->page_count; i++) {
			page = &dsm_g->mem->pages[i];
			if(page->protection & PROT_WRITE) {
				if(satisfy_request(page, &req) < 0) {
					error("error sending GIVEPAGE for terminaison\n");
				}
				giveup_localpage(page, dsm_g->master->sockfd);
			}
		}
	}

	if(dsm_send_msg(dsm_g->master->sockfd, &msg_terminate) < 0) {
		error("error sending TERMINATE\n");
	}

	if(dsm_g->is_master) {
		if (pthread_mutex_lock(&dsm_g->mutex_client_count) < 0) {
			error("lock mutex_client_count");
		}
		while(dsm_g->client_count > 0) {
			if (pthread_cond_wait(&dsm_g->cond_master_end, &dsm_g->mutex_client_count) < 0) {
				error("error pthread_cond_wait mutex_page\n");
			}
		}
		if (pthread_mutex_unlock(&dsm_g->mutex_client_count) < 0) {
			error("unlock mutex_client_count");
		}
	}

	return 0;
}