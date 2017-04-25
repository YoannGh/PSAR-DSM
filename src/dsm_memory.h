#ifndef DSM_MEMORY_H
#define DSM_MEMORY_H

#include <pthread.h>

#include "list.h"

#define MASTER_NODE -8

typedef struct dsm_page_request_s
{
	int sockfd;
	int rights;
} dsm_page_request_t;

typedef struct dsm_page_s
{
	int page_id;
	int protection;
	pthread_mutex_t mutex_page;
	pthread_cond_t cond_uptodate;
	unsigned short uptodate;
	/* Following fields are used by master node only */
	int write_owner;
	list_t *requests_queue;
	list_t *current_readers_queue;
	unsigned short invalidate_sent;
} dsm_page_t;

typedef struct dsm_memory_s
{
	size_t pagesize;
	size_t page_count;
	void* base_addr;
	dsm_page_t *pages;
} dsm_memory_t;

void dsm_memory_init(dsm_memory_t *dsm_mem, size_t pagesize, size_t page_count, 
	unsigned short is_master);

void dsm_memory_destroy(dsm_memory_t *dsm_mem);

//int dsm_add_reader(dsm_memory_t *dsm_mem, unsigned int page_idx, int node_fd);

dsm_page_t* get_page_from_id(unsigned int page_id);

dsm_page_t* get_page_from_addr(void *addr);

#endif