#ifndef DSM_PROTOCOL_H
#define DSM_PROTOCOL_H

#define DSM_MSG_KEY_TYPE "type"
#define DSM_MSG_KEY_BITNESS "bitness"
#define DSM_MSG_KEY_PAGESIZE "pagesize"
#define DSM_MSG_KEY_PAGECOUNT "pagecount"

/**
 * \enum dsm_msg_type_enum
 * \brief enumeration of different messages type.
 **/

typedef enum dsm_msg_type_enum {
	CONNECT,
	CONNECT_ACK,
	GETPAGE,
	INVALIDATE,
	BARRIER,
	TERMINATE,
	ERROR,
	ECHO,
	ECHO_REPLY,
} dsm_msg_type;


/**
 * \struct msg_connect_args_s
 * \brief structure containing connect message arguments
 **/

typedef struct msg_connect_args_s {
	long bitness; /*!< 32 or 64 bit s architecture*/
	long pagesize;/*!< the size of the page for the architecture*/
} msg_connect_args_t;

/**
 * \struct msg_connect_ack_args_s
 * \brief structure containing connect acks message arguments
 **/

typedef struct msg_connect_ack_args_s {
	unsigned short bitness_ok; /*!< flag, true if architecture match*/
	unsigned short pagesize_ok;/*!<flage, true if page size match*/
	long page_count; /*!< number of page on node*/
} msg_connect_ack_args_t;

/**
 * \struct dsm_message_s
 * \brief structure containing message data
 **/

typedef struct dsm_message_s {
	int from_sockfd; /*!< src socket descriptor */
	dsm_msg_type type; /*!< type of message sent */
	union {
		msg_connect_args_t connect_args;
		msg_connect_ack_args_t connect_ack_args;
	} args; /*!< union containing specific message type argument */
	/* union all msg_args */
} dsm_message_t;

void dsm_dispatch_message(dsm_message_t *msg);

int dsm_receive_msg(int nodefd, dsm_message_t *msg);

int dsm_send_msg(int nodefd, dsm_message_t *msg);

#endif