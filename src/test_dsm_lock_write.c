#include <stdio.h>
#include <string.h>

#include "dsm.h"
#include "dsm_util.h"

int main()
{
	char *hello = "Hello world!";
	int i;
	//void *base_addr = InitSlave("132.227.113.162", 5555);
	void *base_addr = InitSlave("127.0.0.1", 5555);

	printf("base_addr: %lx\n", (long) base_addr);

	lock_write(base_addr);
	sync_barrier(3);

	strcpy(base_addr, hello);

	/*printf("R/W, reading: %d\n", *(int *)base_addr);
	i = *(int *)base_addr;
	if(i > 10)
		i = 0;
	else
		i++;

	memcpy(base_addr, &i, sizeof(int));
	printf("write at base_addr: %d\n", *(int *)base_addr);*/

	unlock_write(base_addr);

	QuitDSM();
	return 0;
}
