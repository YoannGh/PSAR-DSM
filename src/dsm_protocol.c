#include "dsm_protocol.h"
#include "dsm_socket.h"
#include "util.h"
#include "binn.h"

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

int dsm_receive_msg(int nodefd, dsm_message_t *msg)
{
	binn *obj;
	void *buffer;
	//char buffer[BUFFER_LEN];
	//void* ptr = (void *) &buffer;

	buffer = malloc(BUFFER_LEN*sizeof(char));

	if(dsm_receive(nodefd, &buffer, BUFFER_LEN) < 0) {
		error("dsm_recv_msg\n");
	}

	obj = binn_open(buffer);

	msg->type = binn_object_int32(obj, DSM_MSG_KEY_TYPE);

	switch (msg->type) {
		default:
        	debug("TODO: msg args deserialization\n");
			break;
	}

	binn_free(obj);
	free(buffer);

	return 0;
}

int dsm_send_msg(int nodefd, dsm_message_t *msg)
{
	binn *obj;
	
	obj = binn_object();
	binn_object_set_int32(obj, DSM_MSG_KEY_TYPE, msg->type);

	if(dsm_send(nodefd, binn_ptr(obj), binn_size(obj)) < 0) {
		log("Could not send to node %d message type: %d\n", nodefd, msg->type);
		return -1;
	}
	binn_free(obj);

	return 0;
}