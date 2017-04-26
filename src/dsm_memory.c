#include <stdlib.h>
#include <sys/mman.h>
#include <pthread.h>

#include "dsm_memory.h"
#include "dsm.h"
#include "dsm_util.h"

extern dsm_t *dsm_g;

int slave_equals(void *slave1, void *slave2)
{
	int fd_slave1 = *(int *)slave1;
	int fd_slave2  = *(int *)slave2;

	return (fd_slave1 == fd_slave2);
}

int request_equals(void* slave1, void* slave2)
{
	return (slave1 == slave2);
}

/**
 * \fn void dsm_memory_init(dsm_memory_t *dsm_mem, size_t pagesize, size_t page_count, unsigned short is_master)
 * \brief Initialisation of substructure memory 
 * \param dsm_mem the structure to be initialised
 * \param pagesize the size of a page for the system
 * \param page_count number of page to be allocated in local memory
 * \param is_master a flag to know if the structure is a master or if is only related to the master
 **/

void dsm_memory_init(dsm_memory_t *dsm_mem, size_t pagesize, size_t page_count,
                    unsigned short is_master)
{
	unsigned int i;
	int prot;

	dsm_mem->pagesize = pagesize;
	dsm_mem->page_count = page_count;

	if (is_master) {
		prot = PROT_READ | PROT_WRITE;
	} else {
		prot = PROT_NONE;
	}

	dsm_mem->base_addr = mmap(NULL, page_count * pagesize, prot, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (dsm_mem->base_addr == NULL) {
		error("Could not allocate memory (mmap)\n");
	}

	dsm_mem->pages = (dsm_page_t *) calloc(page_count, sizeof(dsm_page_t));
	if (dsm_mem->pages == NULL) {
		error("Could not allocate memory (calloc)\n");
	}

	for (i = 0; i < page_count; i++) {
		dsm_mem->pages[i].page_id = i;
		dsm_mem->pages[i].protection = prot;
		pthread_mutex_init(&dsm_mem->pages[i].mutex_page, NULL);
		pthread_cond_init(&dsm_mem->pages[i].cond_uptodate, NULL);
		dsm_mem->pages[i].uptodate = 0;
		if (is_master) {
			dsm_mem->pages[i].uptodate = 1;
			dsm_mem->pages[i].requests_queue = (list_t *) malloc(sizeof(list_t));
			if (dsm_mem->pages[i].requests_queue == NULL) {
				error("Could not allocate memory (malloc)\n");
			}

			dsm_mem->pages[i].current_readers_queue = (list_t *) malloc(sizeof(list_t));
			if (dsm_mem->pages[i].current_readers_queue == NULL) {
				error("Could not allocate memory (malloc)\n");
			}

			list_init(dsm_mem->pages[i].requests_queue, sizeof(dsm_page_request_t), request_equals, NULL);
			list_init(dsm_mem->pages[i].current_readers_queue, sizeof(int), slave_equals, NULL);

			dsm_mem->pages[i].invalidate_sent = 0;
		}
	}
}

/**
 * \fn void dsm_memory_destroy(dsm_memory_t *dsm_mem)
 * \brief free each page allocated then free the structure
 * \param dsm_mem the structure that will be destroyed
 **/

void dsm_memory_destroy(dsm_memory_t *dsm_mem)
{
	unsigned int i;

	for (i = 0; i < dsm_mem->page_count; i++) {
		pthread_mutex_destroy(&dsm_mem->pages[i].mutex_page);
		pthread_cond_destroy(&dsm_mem->pages[i].cond_uptodate);
		if(dsm_g->is_master) {			
			list_destroy(dsm_mem->pages[i].requests_queue);
			list_destroy(dsm_mem->pages[i].current_readers_queue);
			free(dsm_mem->pages[i].requests_queue);
			free(dsm_mem->pages[i].current_readers_queue);
		}
	}
	free(dsm_mem->pages);
	munmap(dsm_mem->base_addr, dsm_mem->page_count*dsm_mem->pagesize);
}

/* FUNCTIONS USED BY MASTER NODE ONLY */

/**
 * \fn dsm_page_t* get_page_from_id(unsigned int page_id)
 * \brief give the adress of a page from its ID
 * \param page_id the page's id
 * \return the base address of the page
 **/
dsm_page_t* get_page_from_id(unsigned int page_id)
{
	if(page_id > (dsm_g->mem->page_count - 1)) {
		log("Wrong page_id: %d\n", page_id);
		return NULL;
	} else {
		return &dsm_g->mem->pages[page_id];
	}
}

/**
 * \fn dsm_page_t* get_page_from_addr(unsigned int page_id)
 * \brief Give the page ID from its the adress
 * \param addr the base address of the page
 * \return the page's id
 **/
dsm_page_t* get_page_from_addr(void *addr)
{
	unsigned int page_id = ((addr - dsm_g->mem->base_addr) / dsm_g->mem->pagesize);

	if(page_id > (dsm_g->mem->page_count - 1)) {
		log("Wrong page_id: %d\n", page_id);
		return NULL;
	} else {
		return get_page_from_id(page_id);
	}
}