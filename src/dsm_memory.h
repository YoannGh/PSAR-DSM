#ifndef DSM_MEMORY_H
#define DSM_MEMORY_H

#include <pthread.h>

#include "list.h"

/**
 * \struct dsm_page_request_s
 * \brief structure representing a request for a page access
 **/
typedef struct dsm_page_request_s
{
	int sockfd; /*!< socket descriptor of the slave requesting */
	int rights; /*!< flag, access type */
} dsm_page_request_t;

/**
 * \struct dsm_page_s
 * \brief structure containing page information
 **/

typedef struct dsm_page_s
{
	unsigned long page_id; /*!< a unique identifier between 0 and nb_page */
	int protection; /*!< current rights on the page */
	pthread_mutex_t mutex_page; /*!< mutex to avoid conflicts between daemon and main thread*/
	pthread_cond_t cond_uptodate; /*!< condition relative to previous mutex*/
	unsigned short uptodate; /*!< flag, true if the page hasn't been invalidated*/
	int write_owner; /*!< sock_fd of the current writer*/
	/* Following fields are used by master node only */
	list_t *requests_queue; /*!< waiting list of all proccess waiting for access */
	list_t *current_readers_queue; /*!< list of all current readers */
	unsigned short invalidate_sent; /*!< flag, true if invalidate message has been sent already once and only one*/
} dsm_page_t;

/**
 * \struct dsm_memory_s
 * \brief information about current memory state settings
 **/

typedef struct dsm_memory_s
{
	size_t pagesize; /*!< the page size for current architecture */
	size_t page_count; /*!< the page number of the dsm */
	void* base_addr; /*!< the starting adress of the dsm */
	dsm_page_t *pages; /*!< the pages of the dsm */
} dsm_memory_t;

void dsm_memory_init(dsm_memory_t *dsm_mem, size_t pagesize, size_t page_count, 
	unsigned short is_master);

void dsm_memory_destroy(dsm_memory_t *dsm_mem);

dsm_page_t* get_page_from_id(unsigned int page_id);

dsm_page_t* get_page_from_addr(void *addr);

int slave_equals(void *slave1, void *slave2);

#endif