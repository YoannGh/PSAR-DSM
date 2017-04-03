#ifndef DSM_H
#define DSM_H

struct s_pageframe
{
	int access_rights;
	void *base_addr;
	int fd_update_owner;
};

struct s_dsm
{
	unsigned long nb_pages;
	unsigned long pagesize;
	void* base_addr;
	struct s_pageframe *pages;
};

void dsm_init(struct s_dsm *dsm, long nb_pages);

void dsm_destroy(struct s_dsm *dsm);

#endif