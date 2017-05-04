#include <stdio.h>
#include <string.h>

#include "dsm.h"
#include "dsm_util.h"

int main()
{
	char *hello = "Hello world!";
	void *base_addr = InitSlave("132.227.112.195", 5555);

	printf("base_addr: %lx\n", (long) base_addr);

	lock_write(base_addr);
	sync_barrier(3);

	strcpy(base_addr, hello);
	printf("\tWrite: %s\n", (char *) base_addr);

	unlock_write(base_addr);

	QuitDSM();
	return 0;
}
