#ifndef GETOPT_SERVER_H 
#define GETOPT_SERVER_H
typedef struct getopt
{
	int			port;
	char		*cert;
	char		*key;
}opt_arg;
opt_arg getopt_server(int argc, char *argv[]);
#endif
