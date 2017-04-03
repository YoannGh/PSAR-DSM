#ifndef UTIL_H
#define UTIL_H

#define DSM_EXIT 	1
#define DSM_NOEXIT 	0

#define handle_error(msg) \
           do { perror(msg); exit(EXIT_FAILURE); } while (0)

void handle_err(char *user_msg, int should_exit);

#endif