#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>

#include "dsm.h"
#include "util.h"

void dsm_init(dsm_t *dsm, long nb_pages)
{
	void *pages_addr;

	dsm->nb_pages = nb_pages;

	dsm->pagesize = sysconf(_SC_PAGE_SIZE);
	if (dsm->pagesize <= 0)
		error("sysconf_pagesize");

	dsm->base_addr = memalign(dsm->pagesize, dsm->nb_pages * dsm->pagesize);
	dsm->pages = (dsm_page_t *) calloc(dsm->nb_pages, sizeof(dsm_page_t));

	pages_addr = dsm->base_addr;
	for (unsigned int i = 0; i < dsm->nb_pages; ++i)
	{
		dsm->pages[i].access_rights = PROT_READ | PROT_WRITE;
		dsm->pages[i].base_addr = pages_addr;
		pages_addr += dsm->pagesize;
	}
}

void dsm_destroy(dsm_t *dsm)
{
	free(dsm->pages);
	free(dsm);
}