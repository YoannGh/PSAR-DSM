#ifndef DSM_PROTOCOL_H
#define DSM_PROTOCOL_H

#define DSM_MSG_KEY_TYPE "type"
#define DSM_MSG_KEY_BITNESS "bitness"
#define DSM_MSG_KEY_PAGESIZE "pagesize"
#define DSM_MSG_KEY_PAGECOUNT "pagecount"
#define DSM_MSG_KEY_PAGEID "pageid"
#define DSM_MSG_KEY_RIGHTS "rights"
#define DSM_MSG_KEY_DATA "data"
#define DSM_MSG_KEY_BARRIER "barrier"

/**
 * \enum dsm_msg_type_enum
 * \brief enumeration of different messages type.
 **/
typedef enum dsm_msg_type_enum {
	CONNECT,
	CONNECT_ACK,
	LOCKPAGE,
	INVALIDATE,
	INVALIDATE_ACK,
	GIVEPAGE,
	SYNC_BARRIER,
	BARRIER_ACK,
	TERMINATE,
} dsm_msg_type;


/**
 * \struct msg_connect_args_s
 * \brief structure containing connect message arguments
 **/

typedef struct msg_connect_args_s {
	unsigned long bitness;
	unsigned long pagesize;
} msg_connect_args_t;

/**
 * \struct msg_connect_ack_args_s
 * \brief structure containing connect acks message arguments
 **/
typedef struct msg_connect_ack_args_s {
	unsigned short bitness_ok; /*!< flag, true if architecture match*/
	unsigned short pagesize_ok;/*!< flag, true if page size match*/
	unsigned long page_count; /*!< number of page on node*/
} msg_connect_ack_args_t;

/**
 * \struct msg_lockpage_args_s
 * \brief structure containing lock message arguments
 **/
typedef struct msg_lockpage_args_s {
	unsigned long page_id; /*!< The page identifier to lock */
	unsigned short access_rights; /*!< The access rights requiered */
} msg_lockpage_args_t;

/**
 * \struct msg_invalidate_args_s
 * \brief structure containing page invalidation message arguments 
 **/
typedef struct msg_invalidate_args_s {
	unsigned long page_id;/*!< The page identifier to invalidate */
} msg_invalidate_args_t;

/**
 * \struct msg_invalidate_ack_args_s
 * \brief structure containing page invalidation ack message arguments  
 **/
typedef struct msg_invalidate_ack_args_s {
	unsigned long page_id;/*!< The page identifier who was successfully invalidate */
} msg_invalidate_ack_args_t;

/**
 * \struct msg_givepage_args_s
 * \brief structure containing givepage message arguments
 **/
typedef struct msg_givepage_args_s {
	unsigned long page_id;/*!< The page identifier to send */
	unsigned short access_rights;/*!< The access rights requiered */
	void* data;/*!< The page itself */
} msg_givepage_args_t;

typedef struct msg_sync_barrier_args_s {
	unsigned short slave_to_wait;/*!< The access rights requiered */
} msg_sync_barrier_args_t;

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
		msg_lockpage_args_t lockpage_args;
		msg_invalidate_args_t invalidate_args;
		msg_invalidate_ack_args_t invalidate_ack_args;
		msg_givepage_args_t givepage_args;
		msg_sync_barrier_args_t sync_barrier_args;
	};
	/* union all msg_args */
} dsm_message_t;

void dsm_dispatch_message(dsm_message_t *msg);

int dsm_receive_msg(int nodefd, dsm_message_t *msg);

int dsm_send_msg(int nodefd, dsm_message_t *msg);

#endif