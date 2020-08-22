#include "getopt_server.h"
#include <getopt.h>
#include <errno.h>
#include <sys/types.h>   
#include <libgen.h>
#include <stdlib.h>
#include <stdio.h>

opt_arg getopt_server(int argc,char *argv[])
{
	int                              				 port_key_cert;
	int								 c=0;
	opt_arg							 	 opt_arg_rt;

	const struct option longopts[]=
	{
		{"port" ,  required_argument,	0,'p'},
		{"cert" ,  required_argument,	0,'c'},
		{"key"  ,  required_argument,	0,'k'},
		{"help" ,  no_argument      ,	0,'h'},
		{NULL	,	0	    ,NULL, 0 }
	};	

	while((c=getopt_long(argc,argv,"k:p:c:h",longopts,NULL))!=-1)
	{
		/*c必须放在while循环内*/
		switch(c)
		{
			case 'p':
				opt_arg_rt.port=atoi(optarg);
				break;

			case 'c':
				opt_arg_rt.cert=optarg;
				break;

			case 'k':
				opt_arg_rt.key=optarg;
				break;

			case 'h':
				printf("IP:-i  [args]\n端口：-p  [args] \n公钥：-k [args]\n证书：-c [args]\n帮助 h\n");
				exit(0);
		}
	}
	port_key_cert=( (!opt_arg_rt.key) ||(!opt_arg_rt.cert) || (!opt_arg_rt.port) );
	if(port_key_cert)
	{
		printf("请port、cert、key同时输入！\n");
		exit(0);
	}

	return opt_arg_rt;
}
