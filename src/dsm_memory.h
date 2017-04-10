#ifndef DSM_MEMORY_H
#define DSM_MEMORY_H

#define READERS_INITIAL_CAPACITY 4
#define MASTER_NODE -8

typedef struct dsm_page_s
{
	int protection;
	/* Following fields are used by master node only */
	int write_owner;
	int readers_count;
	int readers_capacity;
	int *nodes_reading;
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

int dsm_add_reader(dsm_memory_t *dsm_mem, int page_idx, int node_fd);

#endif