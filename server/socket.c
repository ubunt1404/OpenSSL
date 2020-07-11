#include<getopt.h>
#include<stdio.h>
#include<errno.h>
#include<sys/types.h>          
#include<sys/socket.h>
#include<string.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<stdlib.h>
#include "getopt_server.h"
#define  BACKLOG   13
int  main(int   argc,char   **argv)
{

	int 	                      socket_fd=-1;
	int                           acpt_fd=-1;
	int                           rv=-1;
	char						  buf_rv[150];	
	pid_t                         pid=0;
	struct sockaddr_in            serv_addr;
	struct sockaddr_in            cli_addr;
	socklen_t                     cliaddr_len;
	int                           c=0;
	int							  reuse=0;
	int                           port=-1;
	int                           b_rv;
	float                         temp;

	port=getopt_server(argc, argv);
	socket_fd=socket_server_init(NULL,port);
	if(socket_fd<0)
	{
		printf("ERROR: %s server listen on port %d failure\n", argv[0],port);
		return -2;
	}


	setsockopt(socket_fd,SOL_SOCKET,SO_REUSEADDR,(const char*)& reuse,sizeof(int)); /*设置端口复用*/

	while(1)
	{
		/*接收从客户端发来的请求信息*/
		acpt_fd=accept(socket_fd,(struct sockaddr *)&cli_addr,&cliaddr_len);

		while(1)
		{
			rv=read(acpt_fd,buf_rv,sizeof(buf_rv));//<0:出错 ==0：读到文件底部了 >:返回读取的字节数
			if(rv<0)//出错
			{
				printf("read erro:%s\n",strerror(errno));
				close(acpt_fd);
				exit(0);
			}
			else if(rv==0)//读到文件底部
			{
				printf("read break!\n");
				close(acpt_fd);
				exit(0);
			}

			printf("client data is:%s\n",buf_rv);
			if(write(acpt_fd,"server successfully receive!",rv)<0)
			{
				printf("write erro:%s",strerror(errno));
				close(acpt_fd);
			}
		}
		close(acpt_fd);
	}
	return 0;    
}


