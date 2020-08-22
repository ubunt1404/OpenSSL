#include "getopt_client.h"
#include <getopt.h>
#include <errno.h>
#include <sys/types.h>   
#include <libgen.h>
#include <stdlib.h>
#include <stdio.h>
ip_port getopt_client(int argc,char **argv)
{
	ip_port							 ip_po_st;
	int                              			 ip_port;
	unsigned short					 	 port=0;
	char*                            			 ip;
	int							 c=0;

	const struct option longopts[]=
	{
		{"port",  required_argument,0	,'p'},
		{"ip"  ,  required_argument,0	,'i'},
		{"help",  no_argument      ,0	,'h'},
		{NULL  ,	  0        ,NULL, 0 }
	};	
	while((c=getopt_long(argc,argv,"i:p:h",longopts,NULL))!=-1)
	{
		/*c必须放在while循环内*/
		switch(c)
		{
			case 'p':
				ip_po_st.port=atoi(optarg);
				break;
			case 'i':
				ip_po_st.ip=optarg;
				break;
			case 'h':
				printf("端口：--p  [args] \nIP地址：--i [args]\n帮助 h\n");
				exit(0);
		}
	}
	ip_port=((!ip_po_st.ip) ||(!ip_po_st.port));
	if(ip_port)
	{
		printf("请ip 、port同时输入！\n");
		exit(0);
	}
	return ip_po_st;
}
