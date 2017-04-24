#include <stdlib.h>
#include <sys/mman.h>

#include "dsm_memory.h"
#include "dsm_util.h"
#include "list.h"

int slave_equals(void* slave1, void* slave2)
{
	int fd_slave1 = (int) *slave1;
	int fd_slave2  = (int) *slave2;

	if(fd_slave1 == fd_slave2)
		return 1;
	else
		return 0;
}

int request_equals(void* slave1, void* slave2)
{
	if(elem1 == elem2)
		return 1;
	else
		return 0;
}

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
			dsm_mem->pages[i].requests_queue = (list_t *) malloc(sizeof(list_t));
			if (dsm_mem->pages[i].requests_queue == NULL) {
				error("Could not allocate memory (malloc)\n");
			}

			dsm_mem->pages[i].current_readers_queue = (list_t *) malloc(sizeof(list_t));
			if (dsm_mem->pages[i].current_readers_queue == NULL) {
				error("Could not allocate memory (malloc)\n");
			}

			list_init(dsm_mem->pages[i].requests_queue, sizeof(dsm_request_page_t), request_equals, NULL);
			list_init(dsm_mem->pages[i].current_readers_queue, sizeof(int), slave_equals, NULL);

			dsm_mem->pages[i].write_owner = MASTER_NODE;
			dsm_mem->pages[i].invalidate_sent = 0;
		}
	}
}

void dsm_memory_destroy(dsm_memory_t *dsm_mem)
{
	unsigned int i;

	for (i = 0; i < dsm_mem->page_count; i++) {
		free(dsm_mem->pages[i].nodes_reading);
	}
	
	free(dsm_mem->pages);
}

/* FUNCTIONS USED BY MASTER NODE ONLY */

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