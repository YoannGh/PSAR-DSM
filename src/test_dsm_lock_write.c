#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "dsm.h"

int main()
{
	char *hello = "Hello world!";
	void *base_addr = InitSlave("127.0.0.1", 5555);
	printf("base_addr: %lx\n", (long) base_addr);

	lock_write(base_addr);

	strcpy(base_addr, hello);
	printf("write at base_addr: %s\n", (char *) base_addr);
	sleep(20);

	unlock_write(base_addr);

	QuitDSM();
	return 0;
}
	

