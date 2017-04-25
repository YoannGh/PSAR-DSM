#include <stdlib.h>
#include <sys/mman.h>

#include "dsm_memory.h"
#include "dsm_util.h"

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
		dsm_mem->pages[i].protection = prot;
		if (is_master) {
			dsm_mem->pages[i].write_owner = MASTER_NODE;
			dsm_mem->pages[i].readers_count = 1;
			dsm_mem->pages[i].readers_capacity = READERS_INITIAL_CAPACITY;
			dsm_mem->pages[i].nodes_reading = (int *) calloc(READERS_INITIAL_CAPACITY, sizeof(int));
			if (dsm_mem->pages[i].nodes_reading == NULL) {
				error("Could not allocate memory (realloc)\n");
			}
			dsm_mem->pages[i].nodes_reading[0] = MASTER_NODE;
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
		free(dsm_mem->pages[i].nodes_reading);
	}
	
	free(dsm_mem->pages);
}

/* FUNCTIONS USED BY MASTER NODE ONLY */

/**
* \fn static void check_readers_capacity(dsm_page_t *dsm_page)
* \brief check capacity of readers array and double its size if full
* \param dsm_page the page to test capacity
**/

static void check_readers_capacity(dsm_page_t *dsm_page)
{
	if(dsm_page->readers_count >= dsm_page->readers_capacity) {
		dsm_page->readers_capacity *= 2;
		dsm_page->nodes_reading = (int *) realloc(dsm_page->nodes_reading, sizeof(int)*dsm_page->readers_capacity);
		if (dsm_page->nodes_reading == NULL) {
			error("Could not allocate memory (realloc)\n");
		}
	}
}

/**
* \fn int dsm_add_reader(dsm_memory_t *dsm_mem, unsigned int page_idx, int node_fd)
* \brief add a new reader to one page
* \param dsm_mem the structure of memory where is the page
* \param page_idx the index of the page to add the reader
* \param node_fd the descriptor of the reader
**/

int dsm_add_reader(dsm_memory_t *dsm_mem, unsigned int page_idx, int node_fd)
{
	if(page_idx > (dsm_mem->page_count - 1)) {
		log("Wrong page index\n");
		return -1;
	}
	check_readers_capacity(&dsm_mem->pages[page_idx]);

	dsm_mem->pages[page_idx].nodes_reading[dsm_mem->pages[page_idx].readers_count++] = node_fd;

	return 0;
}