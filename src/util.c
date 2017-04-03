#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

void handle_err(char *user_msg, int should_exit)
{
	perror(user_msg);
	if(should_exit)
		exit(EXIT_FAILURE);
}