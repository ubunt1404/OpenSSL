#include "parser.h"
#ifndef	EPOLL_H
#define EPOLL_H

#define MAX_EVENTS          32 

int epoll_init(int socket_fd);
int epoll_to_listen_events(int epoll_fd,int socket_fd,stunnel_t *stunnel);
#endif
