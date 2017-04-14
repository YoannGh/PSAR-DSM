#include <unistd.h>
#include <pthread.h>

#include "dsm.h"
#include "dsm_socket.h"
#include "dsm_protocol.h"
#include "dsm_util.h"

void start_master() 
{
	pthread_t dsm_daemon;
	dsm_t *dsm;

	dsm = malloc(sizeof(dsm_t));

	dsm->is_master = 1;
	dsm->master.host = "localhost";
	dsm->master.port = 5555;

	if (pthread_create(&dsm_daemon, NULL, &dsm_daemon_msg_listener, (void *) dsm) != 0) {
    	error("pthread_create\n");
	}
}

void start_node() 
{
	pthread_t dsm_daemon;
	dsm_t *dsm;

	dsm = malloc(sizeof(dsm_t));

	dsm->is_master = 0;
	dsm->master.host = "localhost";
	dsm->master.port = 5555;

	if (pthread_create(&dsm_daemon, NULL, &dsm_daemon_msg_listener, (void *) dsm) != 0) {
    	error("pthread_create\n");
	}

	sleep(2);
	dsm_message_t echo;
	echo.type = ECHO;
	dsm_send_msg(dsm->master.sockfd, &echo);
	debug("ECHO 1 Sent\n");
	dsm_send_msg(dsm->master.sockfd, &echo);
	debug("ECHO 2 Sent\n");
}

int main(void)
{
	int pid;

	pid = fork();
	if(pid == 0) {
		log("starting master\n");
		start_master();
	} else if(pid > 0) {
		sleep(2);
		pid = fork();
		if(pid == 0) {
			log("starting node 1\n");
			start_node();
		} else if(pid > 0) {
			log("starting node 2\n");
			start_node();
		}
	} else {
		error("fork error\n");
	}

	while(1) 
	{
		sleep(60);
	}

	return 0;
}