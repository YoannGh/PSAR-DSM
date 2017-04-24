#ifndef DSM_PROTOCOL_H
#define DSM_PROTOCOL_H

#define DSM_MSG_KEY_TYPE "type"
#define DSM_MSG_KEY_BITNESS "bitness"
#define DSM_MSG_KEY_PAGESIZE "pagesize"
#define DSM_MSG_KEY_PAGECOUNT "pagecount"
#define DSM_MSG_KEY_PAGEID "pageid"
#define DSM_MSG_KEY_RIGHTS "rights"
#define DSM_MSG_KEY_DATA "data"

typedef enum dsm_msg_type_enum {
	CONNECT,
	CONNECT_ACK,
	LOCKPAGE,
	INVALIDATE,
	INVALIDATE_ACK,
	GIVEPAGE,
	TERMINATE,
} dsm_msg_type;

typedef struct msg_connect_args_s {
	long bitness;
	long pagesize;
} msg_connect_args_t;

typedef struct msg_connect_ack_args_s {
	unsigned short bitness_ok;
	unsigned short pagesize_ok;
	long page_count;
} msg_connect_ack_args_t;

typedef struct msg_lockpage_args_s {
	long page_id;
	unsigned short right_access;
} msg_lockpage_args_t;

typedef struct msg_invalidate_args_s {
	long page_id;
} msg_invalidate_args_t;

typedef struct msg_invalidate_ack_args_s {
	long page_id;
} msg_invalidate_ack_args_t;

typedef struct msg_givepage_args_s {
	long page_id;
	void* data;
} msg_givepage_args_t;

typedef struct dsm_message_s {
	int from_sockfd;
	dsm_msg_type type;
	union {
		msg_connect_args_t connect_args;
		msg_connect_ack_args_t connect_ack_args;
		msg_lockpage_args_t lockpage_args;
		msg_invalidate_args_t invalidate_args;
		msg_invalidate_ack_args_t invalidate_ack_args;
		msg_givepage_args_t givepage_args;
	} args;
	/* union all msg_args */
} dsm_message_t;

void dsm_dispatch_message(dsm_message_t *msg);

int dsm_receive_msg(int nodefd, dsm_message_t *msg);

int dsm_send_msg(int nodefd, dsm_message_t *msg);

#endif