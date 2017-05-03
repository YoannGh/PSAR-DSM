#include <stdlib.h>

#include "dsm.h"
#include "dsm_protocol.h"
#include "dsm_socket.h"
#include "dsm_core.h"
#include "dsm_util.h"
#include "binn.h"

extern dsm_t *dsm_g;
/**
* \fn void dsm_dispatch_message(dsm_message_t *msg)
* \brief function to sort messages by type and call good treatment
* \param msg the message to sort
**/
void dsm_dispatch_message(dsm_message_t *msg)
{

	switch (msg->type) {
		case CONNECT:
			debug("CONNECT Received\n");
			handle_connect_msg(msg->from_sockfd, &msg->connect_args);
			break;
		case CONNECT_ACK:
			log("CONNECT_ACK Received, this shouldnt after connection...\n");
			break;
		case LOCKPAGE:
			debug("LOCKPAGE Received\n");
			handle_lockpage_msg(msg->from_sockfd, &msg->lockpage_args);
			break;
		case INVALIDATE:
			debug("INVALIDATE Received\n");
			handle_invalidate_msg(msg->from_sockfd, &msg->invalidate_args);
			break;
		case INVALIDATE_ACK:
			debug("INVALIDATE_ACK Received\n");
			handle_invalidate_ack_msg(msg->from_sockfd, &msg->invalidate_ack_args);
			break;
		case GIVEPAGE:
			debug("GIVEPAGE Received\n");
			handle_givepage_msg(msg->from_sockfd, &msg->givepage_args);
			break;
		case SYNC_BARRIER:
			debug("SYNC_BARRIER Received\n");
			handle_sync_barrier_msg(msg->from_sockfd, &msg->sync_barrier_args);
			break;
		case BARRIER_ACK:
			debug("BARRIER_ACK Received\n");
			handle_barrier_ack_msg(msg->from_sockfd);
			break;
		case TERMINATE:
			debug("TERMINATE Received\n");
			handle_terminate_msg(msg->from_sockfd);
			break;
		default:
        	log("Unimplemented msg to dispatch, discarding it\n");
			break;
	}
}

/**
* \fn int dsm_receive_msg(int nodefd, dsm_message_t *msg)
* \brief treatment to get message from data received
* \param nodefd message sender descriptor
* \param msg the final message received
**/

int dsm_receive_msg(int nodefd, dsm_message_t *msg)
{
	binn *obj;
	int blob_size;
	void* buffer;
	int buffersize;
	//char buffer[BUFFER_LEN];
	//void* ptr = (void *) &buffer;
	if(dsm_g->mem == NULL) {
		buffersize = MIN_BUFFERSIZE;
	} else {
		buffersize = MIN_BUFFERSIZE + dsm_g->mem->pagesize;
	}
	/* We cant receive a page if the mem struct isn't yet initialized anyway */

	buffer = malloc(buffersize);
	if (buffer == NULL) {
		error("Could not allocate memory (malloc)\n");
	}

	if(dsm_receive(nodefd, buffer) < 0) {
		debug("dsm_receive 0 byte, node disconnected\n");
		return -1;
	}

	obj = binn_open(buffer);

	msg->type = binn_object_int32(obj, DSM_MSG_KEY_TYPE);

	switch (msg->type) {
		case CONNECT:
			msg->connect_args.bitness = binn_object_int32(obj, DSM_MSG_KEY_BITNESS);
			msg->connect_args.pagesize = binn_object_int32(obj, DSM_MSG_KEY_PAGESIZE);
			break;
		case CONNECT_ACK:
			msg->connect_ack_args.bitness_ok = binn_object_int16(obj, DSM_MSG_KEY_BITNESS);
			msg->connect_ack_args.pagesize_ok = binn_object_int16(obj, DSM_MSG_KEY_PAGESIZE);
			msg->connect_ack_args.page_count = binn_object_int32(obj, DSM_MSG_KEY_PAGECOUNT);
			break;
		case LOCKPAGE:
			msg->lockpage_args.page_id = binn_object_int32(obj, DSM_MSG_KEY_PAGEID);
			msg->lockpage_args.access_rights = binn_object_int16(obj, DSM_MSG_KEY_RIGHTS);
			break;
		case INVALIDATE:
			msg->invalidate_args.page_id = binn_object_int32(obj, DSM_MSG_KEY_PAGEID);
			break;
		case INVALIDATE_ACK:
			msg->invalidate_ack_args.page_id = binn_object_int32(obj, DSM_MSG_KEY_PAGEID);
			break;
		case GIVEPAGE:
			msg->givepage_args.page_id = binn_object_int32(obj, DSM_MSG_KEY_PAGEID);
			msg->givepage_args.access_rights = binn_object_int16(obj, DSM_MSG_KEY_RIGHTS);
			msg->givepage_args.data = binn_object_blob(obj, DSM_MSG_KEY_DATA, &blob_size);
			break;
		case SYNC_BARRIER:
			msg->sync_barrier_args.slave_to_wait = binn_object_int16(obj, DSM_MSG_KEY_BARRIER);
			break;
		case BARRIER_ACK:
			break;
		case TERMINATE:
			break;
		default:
        	log("Unknown message type received !\n");
        	return -1;
	}

	binn_free(obj);
	free(buffer);

	return 0;
}

/**
* \fn int dsm_receive_msg(int nodefd, dsm_message_t *msg)
* \brief treatment to send data from message structure
* \param nodefd message destination descriptor
* \param msg the message to send
**/

int dsm_send_msg(int nodefd, dsm_message_t *msg)
{
	binn *obj;
	
	obj = binn_object();
	binn_object_set_int32(obj, DSM_MSG_KEY_TYPE, msg->type);

	switch (msg->type) {
		case CONNECT:
			binn_object_set_int32(obj, DSM_MSG_KEY_BITNESS, msg->connect_args.bitness);
			binn_object_set_int32(obj, DSM_MSG_KEY_PAGESIZE, msg->connect_args.pagesize);
			break;
		case CONNECT_ACK:
			binn_object_set_int16(obj, DSM_MSG_KEY_BITNESS, msg->connect_ack_args.bitness_ok);
			binn_object_set_int16(obj, DSM_MSG_KEY_PAGESIZE, msg->connect_ack_args.pagesize_ok);
			binn_object_set_int32(obj, DSM_MSG_KEY_PAGECOUNT, msg->connect_ack_args.page_count);
			break;
		case LOCKPAGE:
			binn_object_set_int32(obj, DSM_MSG_KEY_PAGEID, msg->lockpage_args.page_id);
			binn_object_set_int16(obj, DSM_MSG_KEY_RIGHTS, msg->lockpage_args.access_rights);
			break;
		case INVALIDATE:
			binn_object_set_int32(obj, DSM_MSG_KEY_PAGEID, msg->invalidate_args.page_id);
			break;
		case INVALIDATE_ACK:
			binn_object_set_int32(obj, DSM_MSG_KEY_PAGEID, msg->invalidate_ack_args.page_id);
			break;
		case GIVEPAGE:
			binn_object_set_int32(obj, DSM_MSG_KEY_PAGEID, msg->givepage_args.page_id);
			binn_object_set_int16(obj, DSM_MSG_KEY_RIGHTS, msg->givepage_args.access_rights);
			binn_object_set_blob(obj, DSM_MSG_KEY_DATA, msg->givepage_args.data, dsm_g->mem->pagesize);
			break;
		case SYNC_BARRIER:
			binn_object_set_int16(obj, DSM_MSG_KEY_BARRIER, msg->sync_barrier_args.slave_to_wait);
		case BARRIER_ACK:
			break;
		case TERMINATE:
			break;
		default:
        	log("Unknown message type to send !\n");
        	return -1;
	}

	if(dsm_send(nodefd, binn_ptr(obj), binn_size(obj)) < 0) {
		log("Could not send to node %d message type: %d\n", nodefd, msg->type);
		return -1;
	}
	binn_free(obj);

	return 0;
}