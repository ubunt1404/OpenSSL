#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "getopt_client.h"

int main(int argc,char **argv)
{
	char                             temp[50]="hello world!";
	int                              sockfd=-1;
	int                              conect_rt=0;
	char                             buf_s[50];
	struct sockaddr_in               serv_addr;
	int                              wrt_rt=0;
	int                              read_rt=0;
	char				             buf_r[50];
	ip_port                          ip_port_st;

	/*openssl初始化*/
	SSL_init();
	
	/*获取ip和端口*/
	ip_port_st=getopt_client(argc,argv);
	
	/*socket初始化*/
	sockfd=socket_init(ip_port_st);


	wrt_rt=write(sockfd,temp,sizeof(temp));
	if(wrt_rt<0)
	{
		printf("write data failure:%s\n",strerror(errno));
		return -3;
	}

	read_rt=read(sockfd,buf_r,sizeof(buf_r));
	if(read_rt<0)
	{
		printf("read failure!\n");
		return -5;
	}
	printf("%s\n",buf_r);
	close(sockfd);
	return 0;
}
