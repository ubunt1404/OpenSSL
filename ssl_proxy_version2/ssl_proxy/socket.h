#ifndef	SOCKET_H
#define	SOCKET_H

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/param.h>
#include <linux/netfilter_ipv4.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <ctype.h>
#include <time.h>
#include <libgen.h>
#include <netinet/in.h> 
#include <sys/epoll.h>
#include <sys/resource.h>

#include <openssl/ssl.h> 
#include <openssl/err.h> 

#include "dictionary.h"
#include "iniparser.h"
#include "parser.h"

#define MAX_EVENTS          32
#define LISTEN_BACKLOG		13

typedef struct map
{
	int							    client_fd;	
	int								server_fd;
	SSL								*client_ssl;
	SSL                             *server_ssl;
}stunnel_map;


int socket_init_listen(int port);
int socket_accept_client(int stl_socket_fd,int port);
int socket_connect_server(char *ip,int port);
SSL *socket_bind_ssl(int server_fd,stunnel_t *stunnel);
int from_server_forward_to_client(stunnel_map *stl_map);
int from_client_forward_to_server(stunnel_map *stl_map);
SSL_CTX *SSL_init_server(char *CAfile,stunnel_t *stunnel);     
SSL_CTX *SSL_init_client(char *CAfile,stunnel_t *stunnel);
void ShowCerts(SSL * ssl);
#endif
