#ifndef DSM_CORE_H
#define DSM_CORE_H

#include "dsm_protocol.h"
#include "dsm_memory.h"

int handle_connect_msg(int from, msg_connect_args_t *args);

int handle_lockpage_msg(int from, msg_lockpage_args_t *args);

int handle_invalidate_msg(int from, msg_invalidate_args_t *args);

int handle_invalidate_ack_msg(int from, msg_invalidate_ack_args_t *args);

int handle_givepage_msg(int from, msg_givepage_args_t *args);

int handle_terminate_msg(int from);

void lock_page(dsm_page_t *page, int rights);

int satisfy_request(dsm_page_t *page, dsm_page_request_t *req);

#endif