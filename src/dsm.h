#ifndef DSM_H
#define DSM_H

typedef struct dsm_master_s
{
	char* host;
	int port;
	int sockfd;
} dsm_master_t;

typedef struct dsm_pageframe_s
{
	int access_rights;
	void *base_addr;
	int fd_update_owner;
} dsm_page_t;

typedef struct dsm_s
{
	unsigned long nb_pages;
	unsigned long pagesize;
	void* base_addr;
	dsm_page_t *pages;
	dsm_master_t master;
	int is_master;
} dsm_t;

void dsm_init(dsm_t *dsm, long nb_pages);

void dsm_destroy(dsm_t *dsm);

#endif