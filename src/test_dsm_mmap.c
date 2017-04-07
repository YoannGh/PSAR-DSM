#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>

#include "dsm.h"
#include "util.h"

int main(void)
{
	int nb_pages = 10;
	dsm_t *dsm;
	void* mapping;

	dsm = (dsm_t *) malloc(sizeof(dsm_t));
	dsm_init(dsm, 10);

	for (int i = 0; i < nb_pages; ++i)
	{
		printf("Page[%d]: mapped at 0x%lx\n", i, (long) dsm->pages[i].base_addr);
		if (((uintptr_t) dsm->pages[i].base_addr % dsm->pagesize) != 0) {
			printf("Page[%d] not aligned on pagesize (%lu)\n", i, dsm->pagesize);
		}
	}

	mapping = mmap(dsm->pages[0].base_addr, dsm->pagesize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
	if (mapping == MAP_FAILED) {
		printf("%d\n", errno);
		if (errno == EACCES)
			puts("lel");
		error("mmap");
	}
	printf("mmap at 0x%lx\n", (long) mapping);

	char test[] = "Test?";
	char y = '!';

	memcpy(mapping, &test, sizeof(test));
	puts("here");
	printf("after memcpy: %s\n", (char *) mapping);
	memcpy(dsm->pages[0].base_addr + 4, &y, 1);
	printf("after memcpy (page): %s\n", (char *) dsm->pages[0].base_addr);
	printf("after memcpy (mapping): %s\n", (char *) mapping);

	puts("mprotect(PROT_NONE)");
	mprotect(dsm->pages[0].base_addr, dsm->pagesize, PROT_NONE);
	puts("Should SegFault here");
	printf("%s\n", (char *) dsm->pages[0].base_addr);

	munmap(dsm->pages[0].base_addr, dsm->pagesize);
	dsm_destroy(dsm);
	return 0;
}

/*
algo replication de pages:

	Pb: la memoire du client n est pas forcement mappée à la même addr
	que celle du serveur (mmap solution ?)

	Taille de cache à initialiser pour les clients ?

	SIGSEGV:
		Demande au serveur la page (en lecture) correspondante
		Si le serveur possède la dernière version:
			Envoie au demandeur
			Ajout à la liste des lecteurs
		Sinon
			Demande au possesseur
			forward la page au demandeur
		A la reception de la page (en lecture)
			mprotect(PROT_READ)

		Demande au serveur la page (en ecriture) correspondante
		Si le serveur possède la dernière version:
			Change le possesseur associé
			Envoie d une invalidation aux lecteurs
			Envoie la page au demandeur
		Sinon
			Demande la page au possesseur
			Change le possesseur associé
			Envoie d une invalidation aux lecteurs
			Envoie la page au demandeur
		A la reception de la page (en ecriture)
			mprotect(PROT_READ | PROT_WRITE)

	à la reception d une demande ou d une invalidation:
		Ajuster les droits avec mprotect accordingly
*/