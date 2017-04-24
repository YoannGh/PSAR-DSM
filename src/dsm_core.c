#include "dsm.h"
#include "dsm_core.h"
#include "dsm_protocol.h"
#include "list.h"

extern dsm_t *dsm_g;

int handle_connect_msg(int from, msg_connect_args_t *args)
{
	dsm_message_t reply;
	reply.type = CONNECT_ACK;

	unsigned short bitness_ok = 0;
	unsigned short pagesize_ok = 0;

	if(args->bitness == sizeof(void*))
		bitness_ok = 1;
	if(args->pagesize == dsm_g->mem->pagesize)
		pagesize_ok = 1;

	msg_connect_ack_args_t ca = {
		.bitness_ok = bitness_ok,
		.pagesize_ok = pagesize_ok,
		.page_count = dsm_g->mem->page_count
	};

	reply.args.msg_connect_ack_args_t = ca;

	return dsm_send_msg(from, &reply);
}


int handle_lockpage_msg(int from, msg_lockpage_args_t *args)
{
	dsm_page_t *page;
	dsm_page_request_t new_req;
	new_req.sockfd = from;
	new_req.rights = args->rights;
	/*TODO FIND PAGE LIST */
	list_add( ,new_req);
	process_list_request();
}

int handle_invalidate_msg(int from, msg_invalidate_args_t *args)
{
	dsm_page_t *page;
	if (pthread_mutex_lock(&page->mutex_page) < 0) {
		error("lock mutex_page");
	}
	page->uptodate = 0;

	if (pthread_mutex_unlock(&page->mutex_page) < 0) {
		error("unlock mutex_page");
	}

	//Send INVALIDATE_ACK
	dsm_message_t reply;
	reply.type = INVALIDATE_ACK;

	if(args->bitness == sizeof(void*))
		bitness_ok = 1;
	if(args->pagesize == dsm_g->mem->pagesize)
		pagesize_ok = 1;

	msg_invalidate_ack_args_t ca = {
		.page_id = args->page_id,
	};

	reply.args.msg_invalidate_ack_args_t = ca;

	return dsm_send_msg(from, &reply);
}

int handle_invalidate_ack_msg(int from, msg_invalidate_ack_args_t *args)
{
//Remove the reader from current_reader list
//process_list_request
}

int handle_givepage_msg(int from, msg_givepage_args_t *args)
{

}

int handle_terminate_msg(int from)
{

}

static int process_list_request(dsm_page_t *page)
{
	listNode *reqlist = page->requests_queue->head;
	dsm_page_request_t req;

	while(reqlist != NULL)
	{
		req = (dsm_page_request_t *) reqlist->data;
		if(req->rights == PROT_READ)
		{
			if(p->write_owner == 0)
			{
				send_page();
				list_add(page->current_readers_queue, &req->sockfd);
				list_remove(page->requests_queue, &req);
				continue;
			}
			else
				return;
		}

		reqlist = reqlist->next;
	}
}
