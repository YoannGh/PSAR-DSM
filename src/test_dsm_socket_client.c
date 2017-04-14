#include <stdio.h>

#include "dsm_socket.h"
#include "dsm_util.h"

int main(void)
{

	if (dsm_socket_connect("127.0.0.1", 5555) > 0) {
		puts("Connected!");
	} else {
		error("error connect\n");
	}
	return 0;
}