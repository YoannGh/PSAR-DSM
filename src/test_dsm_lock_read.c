#include <stdio.h>

#include "dsm.h"

int main()
{
	void *base_addr = InitMaster(5555, 10);
	printf("base_addr: %lx\n", (long) base_addr);
	sync_barrier(3);
	
	lock_read(base_addr);

	printf("\tReading: %s\n", (char *) base_addr);

	unlock_read(base_addr);

	QuitDSM();
	return 0;
}