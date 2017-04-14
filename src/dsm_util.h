#ifndef DSM_UTIL_H
#define DSM_UTIL_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#ifdef DEBUG
	#define VARDEBUG 1
#else
	#define VARDEBUG 0
#endif

/*#define handle_error(msg) \
           do { perror(msg); exit(EXIT_FAILURE); } while (0)*/

#define debug_print(condition, stream, ...) \
        do { if (condition) fprintf(stream, "%s:%d:%s(): ", __FILE__, \
                                __LINE__, __func__); \
                            fprintf(stream, __VA_ARGS__); } while (0)

#define debug(...) debug_print(VARDEBUG, stdout, __VA_ARGS__)

#define log(...) debug_print(1, stdout, __VA_ARGS__)

#define error(...) \
        do { debug_print(1, stderr, __VA_ARGS__); perror("\t"); exit(EXIT_FAILURE); } while (0)

#endif