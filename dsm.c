
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <errno.h>

#include "dsm.h"

#define handle_error(msg) \
           do { perror(msg); exit(EXIT_FAILURE); } while (0)


int main(int argc, char *argv[])
{
	int nb_pages = 10;
	struct s_dsm *dsm;
	
	dsm = (struct s_dsm *) malloc(sizeof(struct s_dsm));
	dsm_init(dsm, 10);

	for (int i = 0; i < nb_pages; ++i)
	{
		printf("Page[%d]: mapped at 0x%lx\n", i, (long) dsm->pages[i].base_addr);
	}

	dsm_destroy(dsm);
	return 0;
}

void dsm_init(struct s_dsm *dsm, long nb_pages) 
{
	void *pages_addr;
	dsm->nb_pages = nb_pages;

	dsm->pagesize = sysconf(_SC_PAGE_SIZE);
	if (dsm->pagesize == -1)
		handle_error("sysconf_pagesize");

	dsm->base_addr = memalign(dsm->pagesize, dsm->nb_pages * dsm->pagesize);
	dsm->pages = (struct s_pageframe *) calloc(dsm->nb_pages, sizeof(struct s_pageframe));

	pages_addr = dsm->base_addr;
	for (int i = 0; i < dsm->nb_pages; ++i)
	{
		dsm->pages[i].access_rights = PROT_NONE;
		dsm->pages[i].base_addr = pages_addr;
		pages_addr += dsm->pagesize;
	}
}

void dsm_destroy(struct s_dsm *dsm)
{
	free(dsm->pages);
	free(dsm);
}