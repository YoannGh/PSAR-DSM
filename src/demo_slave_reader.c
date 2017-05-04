#include <stdio.h>

#include "dsm.h"

int main()
{
	unsigned int nb_proc = 3;
	unsigned int nb_lecture = 200;

	void *base_addr;
	int *entier1;
	int *entier2;
	int *entier3;

	base_addr = InitSlave("127.0.0.1", 5555);

	entier1 = base_addr;
	entier2 = base_addr + 13337;
	entier3 = base_addr + 33333;

	sync_barrier(nb_proc);
	for(unsigned int i = 0; i < nb_lecture; i++) {
		lock_read(entier1);
		printf("entier1 = %d\n", (*entier1));
		unlock_read(entier1);
		lock_read(entier2);
		printf("entier2 = %d\n", (*entier2));
		unlock_read(entier2);
		lock_read(entier3);
		printf("entier3 = %d\n", (*entier3));
		unlock_read(entier3);
	}
	sync_barrier(nb_proc);

	QuitDSM();
	return 0;
}