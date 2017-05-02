#include <stdio.h>
#include <unistd.h>

#include "dsm.h"

int main()
{
	void *base_addr = InitSlave("132.227.113.162", 5555);
	printf("base_addr: %lx\n", (long) base_addr);
	
	sleep(30);

	QuitDSM();
	return 0;
}