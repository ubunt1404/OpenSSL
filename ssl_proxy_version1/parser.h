#ifndef PARSER_H
#define PARSER_H

#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>


#include "dictionary.h"
#include "iniparser.h"

typedef struct
{	
	int									client_port;
	char								*server_ip;
	int									server_port;
	char								*CAfile;
}stunnel_t;

extern int                              count;                                      
extern int                              g_sigstop;


/*get conf file path*/
char *parser_argument(int argc, char **argv);
void print_usage();

/*parser the conf file to get client_port\server_ip\server_port\CAfile storage in stunnel[i]*/
int conf_parser(char *path,int max,stunnel_t *stunnel);


#endif 

