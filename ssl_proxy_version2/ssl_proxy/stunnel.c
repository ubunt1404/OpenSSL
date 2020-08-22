#include "parser.h"
#include "socket.h"
#include "epoll.h"
#include "thread.h"

#define MAX					 5


int main(int argc,char *argv[])
{
	int									port_cli=0;
	int									port_sev=0;
	int									socket_fd_cli=0;
	int                                 					socket_fd_sev=0;
	struct sockaddr_in                  					client_dest_addr;
	int								    	client_fd=0;
	SSL_CTX *								ctx;
	int									len=0;
	struct sockaddr_in							dest;
	char									buffer[128];
	SSL									*ssl;

	int									i=0;
	int									rv=0;
	stunnel_t								stunnel[MAX];					
	char							    		*conf_path=NULL;

	/*argument parser to get conf file path*/
	conf_path=parser_argument(argc,argv);

	/*parser ini conf file */
	conf_parser(conf_path,MAX,stunnel);
	
	for(i=0;i<MAX;i++)
	{
		printf("stunnel[%d].server_port is:%d\n", i,stunnel[i].server_port);
		printf("stunnel[%d].server_ip is:%s\n", i,stunnel[i].server_ip);
		printf("stunnel[%d].client_port is:%d\n", i,stunnel[i].client_port);
		
		if((stunnel[i].server_port==0)&&(!stunnel[i].server_ip)&&(stunnel[i].client_port==0))
		{
			continue;
		}
		else
		{
			rv=create_thread_to_listen(&stunnel[i]);
			if(rv<0)
			{
				printf("child thread init failure!\n");
				continue;
			}
		}
	}
}

