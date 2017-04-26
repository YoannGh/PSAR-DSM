#include <stdio.h>
#include <string.h>

#include "dsm.h"

int main()
{
	void *base_addr = InitSlave("127.0.0.1", 5555);
	printf("base_addr: %lx\n", (long) base_addr);
	sync_barrier(3);

	sync_barrier(3);
	lock_read(base_addr);

	printf("read at base_addr: %s\n", (char *) base_addr);

	unlock_read(base_addr);

	sync_barrier(3);
	QuitDSM();
	return 0;
}