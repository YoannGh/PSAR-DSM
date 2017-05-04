#include <stdio.h>
#include <string.h>

#include "dsm.h"

int main()
{
	int nb_proc = 3;
	char *hello = "Hello world!";
	//void *base_addr = InitSlave("132.227.112.195", 5555);
	void *base_addr = InitSlave("127.0.0.1", 5555);
	printf("base_addr: %lx\n", (long) base_addr);
	
	lock_write(base_addr);
	sync_barrier(nb_proc);

	strcpy(base_addr+sizeof(int), hello);
	printf("\tWriting: %s\n", base_addr+sizeof(int));

	unlock_write(base_addr);

	sync_barrier(nb_proc);

	lock_read(base_addr);
	printf("entier = %d\n", *((int*) (base_addr)));
	printf("chaine = %s\n", base_addr + sizeof(int));
	unlock_read(base_addr);

	QuitDSM();
	return 0;
}