#include "dsm_protocol.h"
#include "dsm_socket.h"
#include "dsm_util.h"
#include "binn.h"

/**
* \fn void dsm_dispatch_message(dsm_message_t *msg)
* \brief function to sort messages by type and call good treatment
* \param msg the message to sort
**/

void dsm_dispatch_message(dsm_message_t *msg)
{
	dsm_message_t reply;

	switch (msg->type) {
		case CONNECT:
			debug("CONNECT Received\n");
			free(msg);
			break;
		case GETPAGE:
			debug("GETPAGE Received\n");
			free(msg);
			break;
		case INVALIDATE:
			debug("INVALIDATE Received\n");
			free(msg);
			break;
		case BARRIER:
			debug("BARRIER Received\n");
			free(msg);
			break;
		case TERMINATE:
			debug("TERMINATE Received\n");
			free(msg);
			break;
		case ERROR:
			debug("ERROR Received\n");
			free(msg);
			break;
		case ECHO:
			reply.type = ECHO_REPLY;
			dsm_send_msg(msg->from_sockfd, &reply);
			debug("ECHO Received\n");
			free(msg);
			break;
		case ECHO_REPLY:
			debug("ECHO_REPLY Received\n");
			free(msg);
			break;
		default:
        	debug("Unimplemented msg Received\n");
			free(msg);
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
		default:
        	debug("TODO: msg args deserialization\n");
			break;
	}

	binn_free(obj);

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
			binn_object_set_int32(obj, DSM_MSG_KEY_BITNESS, msg->args.connect_args.bitness);
			binn_object_set_int32(obj, DSM_MSG_KEY_PAGESIZE, msg->args.connect_args.pagesize);
			break;
		case CONNECT_ACK:
			binn_object_set_int16(obj, DSM_MSG_KEY_BITNESS, msg->args.connect_ack_args.bitness_ok);
			binn_object_set_int16(obj, DSM_MSG_KEY_PAGESIZE, msg->args.connect_ack_args.pagesize_ok);
			binn_object_set_int32(obj, DSM_MSG_KEY_PAGECOUNT, msg->args.connect_ack_args.page_count);
			break;
		default:
        	debug("TODO: msg args serialization\n");
			break;
	}

	if(dsm_send(nodefd, binn_ptr(obj), binn_size(obj)) < 0) {
		log("Could not send to node %d message type: %d\n", nodefd, msg->type);
		return -1;
	}
	binn_free(obj);

	return 0;
}