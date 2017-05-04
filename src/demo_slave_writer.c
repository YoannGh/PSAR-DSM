#include <stdio.h>
#include <string.h>

#include "dsm.h"

int main()
{
	int entier = 42;
	int nb_proc = 3;
	int *entier_ptr;

	void *base_addr = InitSlave("127.0.0.1", 5555);
	printf("base_addr: %lx\n", (long) base_addr);
	entier_ptr = (int *) base_addr;
	printf("entier_ptr: %lx\n", (long) entier_ptr);
	
	sync_barrier(nb_proc);
	lock_write(base_addr);

	memcpy(entier_ptr, &entier, sizeof(int));
	printf("Here\n");
	printf("\tWriting: %d\n", *((int*) (base_addr)));

	unlock_write(base_addr);

	sync_barrier(nb_proc);
	QuitDSM();
	return 0;
}