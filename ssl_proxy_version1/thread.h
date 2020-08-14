#ifndef	THREAD_H
#define	THREAD_H
#include <pthread.h>
#include "epoll.h"

int create_thread_to_listen(stunnel_t *stunnel);
void* work_stunnel(void* arg) ;
void signal_stop(int signum);
#endif
