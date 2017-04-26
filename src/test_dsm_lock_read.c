#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "dsm.h"

int main()
{
	void *base_addr = InitMaster(5555, 10);
	printf("base_addr: %lx\n", (long) base_addr);
	sleep(10);

	lock_read(base_addr);

	printf("read at base_addr: %s\n", (char *) base_addr);

	unlock_read(base_addr);

	sleep(30);
	QuitDSM();
	return 0;
}