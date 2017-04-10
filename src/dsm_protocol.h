#ifndef DSM_PROTOCOL_H
#define DSM_PROTOCOL_H

#define DSM_MSG_KEY_TYPE "type"

typedef enum dsm_msg_type_enum {
	CONNECT,
	GETPAGE,
	INVALIDATE,
	BARRIER,
	TERMINATE,
	ERROR,
	ECHO,
	ECHO_REPLY,
} dsm_msg_type;

typedef struct dsm_message_s {
	int from_sockfd;
	dsm_msg_type type;
} dsm_message_t;

void dsm_dispatch_message(dsm_message_t *msg);

int dsm_receive_msg(int nodefd, dsm_message_t *msg);

int dsm_send_msg(int nodefd, dsm_message_t *msg);

#endif