#include <stdio.h>
#include <string.h>

#include "dsm.h"

int main()
{
	char *hello = "Hello world!";
	void *base_addr = InitSlave("127.0.0.1", 5555);
	printf("base_addr: %lx\n", (long) base_addr);
	sync_barrier(3);

	lock_write(base_addr);
	sync_barrier(3);

	strcpy(base_addr, hello);
	printf("write at base_addr: %s\n", (char *) base_addr);

	unlock_write(base_addr);

	sync_barrier(3);
	QuitDSM();
	return 0;
}
	

