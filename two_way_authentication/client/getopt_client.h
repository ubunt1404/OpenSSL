#ifndef GETOPT_CLIENT_H
#define GETOPT_CLIENT_H
typedef struct ip_port_struct
{
	int			port;
	char		*ip;
	char		*cert;
	char		*key;
}opt_arg;
opt_arg getopt_client(int argc,char *argv[]);
#endif
