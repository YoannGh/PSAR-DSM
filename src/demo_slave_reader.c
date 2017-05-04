#include <stdio.h>

#include "dsm.h"

int main()
{
	unsigned int nb_proc = 3;
	unsigned int nb_lecture = 1;

	//void *base_addr = InitSlave("132.227.112.195", 5555);
	void *base_addr = InitSlave("127.0.0.1", 5555);
	printf("base_addr: %lx\n", (long) base_addr);

	sync_barrier(nb_proc);
	for(unsigned int i = 0; i < nb_lecture; i++) {
		lock_read(base_addr);
		printf("entier = %d\n", *((int*) (base_addr)));
		printf("chaine = %s\n", base_addr + sizeof(int));
		unlock_read(base_addr);
	}

	sync_barrier(nb_proc);
	QuitDSM();
	return 0;
}