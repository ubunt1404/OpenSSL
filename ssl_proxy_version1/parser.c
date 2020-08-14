#include <string.h>
#include "parser.h"
#include <sys/types.h>                                                                 
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>

#include "dictionary.h"
#include "iniparser.h"
int conf_parser(char *path,int max,stunnel_t *stunnel)
{
	dictionary							*ini= NULL;
	char								key[64];
	int									i=0;
	char								*str_ip=NULL;
	char								*str_ip_point=NULL;
	int									client_port;
	char								*str_CA=NULL;

	if(!stunnel)
	{
		printf("formal argument 'stunnel_t *stunnel' invalid!\n");
		exit(1);
	}
	ini = iniparser_load(path);
	if( ini ==NULL)
	{
		printf("load conf file failure!\n");
		return -1;
	}

	for(i=0;i<max;i++)
	{
		memset(&stunnel[i],0,sizeof(stunnel[i]));

		/*parser server port 、server ip */
		snprintf(key,sizeof(key),"stunnel%d:connect",i+1);
		str_ip=iniparser_getstring(ini,key, "null");
		str_ip_point=strstr(str_ip,":");



		/*parser client port */
		snprintf(key,sizeof(key),"stunnel%d:accept",i+1);
		client_port=iniparser_getint(ini,key,3);

		/*parser CAfile*/
		snprintf(key,sizeof(key),"stunnel%d:CAfile",i+1);
		str_CA=iniparser_getstring(ini,key, "null");

		if( str_ip&&client_port )
		{
			printf("stunnel%d parser is ok!\n",i+1);
			printf("tunnel%d:[%d====>%s]\n",i+1,client_port,str_ip);
			printf("CAfile is :%s\n", str_CA);
		}
		else
			continue;/*if stunnel[i] parser error,continue parser next stunnel*/

		/*load data to struct stunnel[i] */
		stunnel[i].server_port =atoi(str_ip_point+1);	
		stunnel[i].server_ip = strtok(str_ip,":");
		stunnel[i].client_port = client_port;
		stunnel[i].CAfile=str_CA;
	}
}


char * parser_argument(int argc, char **argv)
{
	int									c=0;
	int									digit_optind = 0;
	char								*path=NULL;

	/*function argument valid check. if only has argument './test' print_usage*/
	if(argc==1)
	{
		print_usage();
		exit(0);
	}

	const struct option long_options[] = 
	{
		{"help"	  ,		   no_argument, 0,  'h' },
		{"version",		   no_argument, 0,  'v' },
		{"conf"   ,  required_argument, 0,  'c' },
		{0		  ,					 0, 0,   0  }
	};

	while( (c=getopt_long(argc, argv, "c:hv",long_options, NULL))!=-1 )
	{
		switch (c) 
		{
			case 'c':
				path=optarg;
				break;
			case 'h':
				print_usage();
				break;
			case 'v':
				printf("stunnel version 1.0\n");
				break;
		}
	}

	if((path==NULL))
	{
		print_usage();
		exit(0);
	}
	else 
		printf("get path is:%s\n",path);

	return path;
}

void print_usage()
{
	printf("Usage:\n");
	printf("	Conf file path:-c,--conf [args]\n");
	printf("	Version:-v,--version\n");
	printf("	Help:-h,--help\n");
}

