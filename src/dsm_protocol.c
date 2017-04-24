#include "dsm_protocol.h"
#include "dsm_socket.h"
#include "dsm_core.h"
#include "dsm_util.h"
#include "binn.h"

void dsm_dispatch_message(dsm_message_t *msg)
{

	switch (msg->type) {
		case CONNECT:
			debug("CONNECT Received\n");
			handle_connect_msg(msg->from_sockfd, &msg->args.connect_args);
			break;
		case CONNECT_ACK:
			log("CONNECT_ACK Received, this shouldnt happen...\n");
			break;
		case LOCKPAGE:
			debug("LOCKPAGE Received\n");
			handle_lockpage_msg(msg->from_sockfd, &msg->args.lockpage_args);
			break;
		case INVALIDATE:
			debug("INVALIDATE Received\n");
			handle_invalidate_msg(msg->from_sockfd, &msg->args.invalidate_args);
			break;
		case INVALIDATE_ACK:
			debug("INVALIDATE_ACK Received\n");
			handle_invalidate_ack_msg(msg->from_sockfd, &msg->args.invalidate_ack_args);
			break;
		case GIVEPAGE:
			debug("GIVEPAGE Received\n");
			handle_givepage_msg(msg->from_sockfd, &msg->args.givepage_args);
			break;
		case TERMINATE:
			debug("TERMINATE Received\n");
			handle_terminate_msg(msg->from_sockfd);
			break;
		default:
        	debug("Unimplemented msg Received\n");
			break;
	}
}

int dsm_receive_msg(int nodefd, dsm_message_t *msg)
{
	binn *obj;
	char buffer[BUFFER_LEN];
	void* ptr = (void *) &buffer;

	if(dsm_receive(nodefd, &ptr) < 0) {
		error("dsm_recv_msg\n");
	}

	obj = binn_open(buffer);

	msg->type = binn_object_int32(obj, DSM_MSG_KEY_TYPE);

	switch (msg->type) {
		case CONNECT:
			msg->args.connect_args.bitness = binn_object_int32(obj, DSM_MSG_KEY_BITNESS);
			msg->args.connect_args.pagesize = binn_object_int32(obj, DSM_MSG_KEY_PAGESIZE);
			break;
		case CONNECT_ACK:
			msg->args.connect_ack_args.bitness_ok = binn_object_int16(obj, DSM_MSG_KEY_BITNESS);
			msg->args.connect_ack_args.pagesize_ok = binn_object_int16(obj, DSM_MSG_KEY_PAGESIZE);
			msg->args.connect_ack_args.page_count = binn_object_int32(obj, DSM_MSG_KEY_PAGECOUNT);
			break;
		case LOCKPAGE:
			msg->args.lockpage_args.page_id = binn_object_int32(obj, DSM_MSG_KEY_PAGEID);
			msg->args.lockpage_args.right_access = binn_object_int16(obj, DSM_MSG_KEY_RIGHTS);


		case INVALIDATE:
			msg->args.invalidate_args.page_id = binn_object_int32(obj, DSM_MSG_KEY_PAGEID);
			break;
		case INVALIDATE_ACK:
			msg->args.invalidate_ack_args.page_id = binn_object_int32(obj, DSM_MSG_KEY_PAGEID);
			break;
		case GIVEPAGE:
			msg->args.givepage_args.page_id = binn_object_int32(obj, DSM_MSG_KEY_PAGEID);
			msg->args.givepage_args.data = binn_object_str(obj, DSM_MSG_KEY_DATA);
			break;
		case TERMINATE:
			break;
		default:
        	log("Unknown Message recv type !\n");
        	return -1;
	}

	binn_free(obj);

	return 0;
}

int dsm_send_msg(int nodefd, dsm_message_t *msg)
{
	binn *obj;
	
	obj = binn_object();
	binn_object_set_int32(obj, DSM_MSG_KEY_TYPE, msg->type);

	switch (msg->type) {
		case CONNECT:
			binn_object_set_int32(obj, DSM_MSG_KEY_BITNESS, msg->args.connect_args.bitness);
			binn_object_set_int32(obj, DSM_MSG_KEY_PAGESIZE, msg->args.connect_args.pagesize);
			break;
		case CONNECT_ACK:
			binn_object_set_int16(obj, DSM_MSG_KEY_BITNESS, msg->args.connect_ack_args.bitness_ok);
			binn_object_set_int16(obj, DSM_MSG_KEY_PAGESIZE, msg->args.connect_ack_args.pagesize_ok);
			binn_object_set_int32(obj, DSM_MSG_KEY_PAGECOUNT, msg->args.connect_ack_args.page_count);
			break;
		case LOCKPAGE:
			binn_object_set_int32(obj, DSM_MSG_KEY_PAGEID, msg->args.lockpage_args.page_id);
			binn_object_set_int16(obj, DSM_MSG_KEY_RIGHTS, msg->args.lockpage_args.right_access);
			break;
		case INVALIDATE:
			binn_object_set_int32(obj, DSM_MSG_KEY_PAGEID, msg->args.invalidate_args.page_id);
			break;
		case INVALIDATE_ACK:
			binn_object_set_int32(obj, DSM_MSG_KEY_PAGEID, msg->args.invalidate_ack_args.page_id);
			break;
		case GIVEPAGE:
			binn_object_set_int32(obj, DSM_MSG_KEY_PAGEID, msg->args.givepage_args.page_id);
			binn_object_set_str(obj, DSM_MSG_KEY_DATA, msg->args.givepage_args.page_id);
			break;
		case TERMINATE:
			break;

		default:
        	log("Unknown Message sent type !\n");
        	return -1;
	}

	if(dsm_send(nodefd, binn_ptr(obj), binn_size(obj)) < 0) {
		log("Could not send to node %d message type: %d\n", nodefd, msg->type);
		return -1;
	}
	binn_free(obj);

	return 0;
}