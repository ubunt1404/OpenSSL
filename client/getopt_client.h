#ifndef GETOPT_CLIENT_H
#define GETOPT_CLIENT_H
typedef struct ip_port_struct
{
	int			port;
	char		*ip;
}ip_port;
ip_port getopt_client(int argc,char *argv[]);
#endif
