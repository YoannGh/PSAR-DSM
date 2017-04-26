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


int handle_lockpage_msg(int from, msg_lockpage_args_t *args)
{
	dsm_page_t *page;
	dsm_page_request_t new_req = {
		.sockfd = from,
		.rights = args->access_rights
	};

	if(args->access_rights & PROT_WRITE)
		debug("Received LOCKWRITE on page %lu from %d\n", args->page_id, from);
	else if(args->access_rights == PROT_READ)
		debug("Received LOCKREAD on page %lu from %d\n", args->page_id, from);
	else
		debug("Received unknown LOCK on page %lu from %d\n", args->page_id, from);
	 
	page = get_page_from_id(args->page_id);

	list_add(page->requests_queue, &new_req);
	process_list_requests(page);

	return 0;
}

int handle_invalidate_msg(int from, msg_invalidate_args_t *args)
{
	dsm_page_t *page;
	void* page_base_addr;

	page = get_page_from_id(args->page_id);
	page_base_addr = dsm_g->mem->base_addr + (args->page_id * dsm_g->mem->pagesize);

	if (pthread_mutex_lock(&page->mutex_page) < 0) {
		error("lock mutex_page");
	}

	page->uptodate = 0;
	if(mprotect(page_base_addr, dsm_g->mem->pagesize, PROT_NONE) < 0) {
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

int handle_invalidate_ack_msg(int from, msg_invalidate_ack_args_t *args)
{
	dsm_page_t *page;

	page = get_page_from_id(args->page_id);

	list_remove(page->current_readers_queue, &from);
	process_list_requests(page);

	return 0;
}

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
	if(mprotect(page_base_addr, dsm_g->mem->pagesize, PROT_READ|PROT_WRITE) < 0) {
		error("error mprotect\n");
	}

	memcpy(page_base_addr, args->data, dsm_g->mem->pagesize);

	if(mprotect(page_base_addr, dsm_g->mem->pagesize, args->access_rights) < 0) {
		error("error mprotect\n");
	}

	page->protection = args->access_rights;
	page->write_owner = dsm_g->master->sockfd;
	page->uptodate = 1;
	
	if(pthread_cond_signal(&page->cond_uptodate) < 0) {
		error("error pthread_cond_signal\n");
	}

	if (pthread_mutex_unlock(&page->mutex_page) < 0) {
		error("unlock mutex_page");
	}

	return 0;
}

void lock_page(dsm_page_t *page, int rights)
{

	if (pthread_mutex_lock(&page->mutex_page) < 0) {
		error("lock mutex_page");
	}

	if(!page->uptodate) {

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

		while(!page->uptodate) {
			if(pthread_cond_wait(&page->cond_uptodate, &page->mutex_page) < 0) {
				error("error pthread_cond_wait\n");
			}
		}
	}
}

int handle_terminate_msg(int from)
{
	int ret;
	ret = dsm_socket_shutdown(from, SHUT_RDWR);
	return ret && dsm_socket_close(from);
}

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

	debug("Sending page %lu to %d\n", page->page_id, req->sockfd);
	return dsm_send_msg(req->sockfd, &givepage_msg);
}

static int send_invalidate(dsm_page_t *page, int to) 
{
	dsm_message_t msg_invalidate;
	msg_invalidate.type = INVALIDATE;

	msg_invalidate_args_t ia = {
		.page_id = page->page_id,
	};

	msg_invalidate.invalidate_args = ia;

	return dsm_send_msg(to, &msg_invalidate);
}

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
			if(satisfy_request(page, req) < 0) {
				log("error satisfy_request\n");
			}
			reqlist = reqlist->next;
			list_add(page->current_readers_queue, &req->sockfd);
			list_remove(page->requests_queue, &req);
			continue;
		}
		//Case it's a writer, if there is some readers we must ask them to invalidate
		//their pages, and wait for all of them to realease their lock (ack they know the
		//file is dirty)
		else if (req->rights & PROT_WRITE)
		{
			if (!page->invalidate_sent && page->current_readers_queue->length > 0)
			{
				if(send_invalidate(page, req->sockfd) < 0) {
					log("error send_invalidate\n");
				}
				page->invalidate_sent = 1;
			}
			else if (page->current_readers_queue->length == 0)
			{
				page->invalidate_sent = 0;
				page->write_owner = req->sockfd;
				page->uptodate = 0;
				if(satisfy_request(page, req) < 0) {
					log("error satisfy_request\n");
				}
				list_remove(page->requests_queue, &req);
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
