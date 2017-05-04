#include <stdio.h>

#include "dsm.h"

int main()
{
	int entier = 42;
	int nb_proc = 3;
	int *entier_ptr;

	void *base_addr = InitMaster(5555, 10);
	printf("base_addr: %lx\n", (long) base_addr);
	entier_ptr = (int *) base_addr;
	
	lock_write(base_addr);
	sync_barrier(nb_proc);

	(*entier_ptr) = entier;
	printf("\tWriting: %d\n", *((int*) (base_addr)));

	unlock_write(base_addr);

	sync_barrier(nb_proc);
	QuitDSM();
	return 0;
}