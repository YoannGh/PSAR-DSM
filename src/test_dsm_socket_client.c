#include <stdio.h>

#include "dsm_socket.h"
#include "util.h"

int main(void)
{
	
	if(dsm_socket_connect("127.0.0.1", 5555) > 0) {
		puts("Connected!");
	} else {
		handle_err("error connect", DSM_EXIT);
	}

	return 0;
}