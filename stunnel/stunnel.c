#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/param.h>
#include <linux/netfilter_ipv4.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <ctype.h>
#include <time.h>
#include <libgen.h>
#include <netinet/in.h> 
#include <sys/epoll.h>
#include <sys/resource.h>

#include <openssl/ssl.h> 
#include <openssl/err.h> 

#include "dictionary.h"
#include "iniparser.h"

#define PATH "/home/google/workspace/program_stunnel/stunnel/stunnel.conf"
#define MAX  5
#define LISTEN_BACKLOG 13
#define MAX_EVENTS          32
#define ARRAY_SIZE(x)       (sizeof(x)/sizeof(x[0]))

typedef struct
{	
	int								client_port;
	char							*server_ip;
	int								server_port;
	char							*CAfile;
}str_stunnel;

typedef struct map
{
	int								client_fd;
	int								server_fd;
	SSL								*ssl;
}stunnel_map;

int conf_parser(char *path,int max,str_stunnel *ip_port_stunnel);
int stunnel_socket_init_to_client(int port);
int stunnel_get_client_fd(int stl_socket_fd,int port);
SSL_CTX * SSL_init(str_stunnel *ip_port_stunnel);
void ShowCerts(SSL * ssl);
int stunnel_socket_init_to_server(char *ip,int port);
void* work_func(void* arg);

int create_thread_to_listen(str_stunnel *ip_port_stunnel);
int epoll_init(int socket_fd);
int epoll_to_listen_events(int epoll_fd,int socket_fd,str_stunnel *ip_port_stunnel);
SSL * stunnel_ssl_init_to_server(stunnel_map *stl_map,str_stunnel *ip_port_stunnel);
int transmit(stunnel_map *stl_map,struct epoll_event *event_array,int events);

static int							count=0;
int main(int argc,char *argv[])
{
	int									port_cli=0;
	int									port_sev=0;
	int									socket_fd_cli=0;
	int                                 socket_fd_sev=0;
	struct sockaddr_in                  client_dest_addr;
	int								    client_fd=0;
	SSL_CTX *							ctx;
	int									len=0;
	struct sockaddr_in					dest;
	char								buffer[128];
	SSL									*ssl;

	int									i=0;
	int									parser_rt=0;
	str_stunnel							ip_port_stunnel[MAX];					

	/*parser ini conf file */
	parser_rt=conf_parser(PATH,MAX,ip_port_stunnel);
	if(parser_rt<0)
		printf("parser ini file failture!\n");

	for(i=1;i<=MAX;i++)
	{
		printf("stunnel[%d].server_port is:%d\n", i,ip_port_stunnel[i].server_port);
		printf("stunnel[%d].server_ip is:%s\n", i,ip_port_stunnel[i].server_ip);
		printf("stunnel[%d].client_port is:%d\n", i,ip_port_stunnel[i].client_port);

		create_thread_to_listen(&ip_port_stunnel[i]);
	}
}


int stunnel_socket_init_to_client(int port) 
{
	int									sockfd=0;
	int									on = 1;
	struct sockaddr_in					serv_addr;
	struct sockaddr_in                  client_dest_addr;
	int								    client_fd=0;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("stunnel fail to create socket !\n");
	}

	printf("stunnel created socket to client \n");
	/*下一次重用这个sockfd地址*/
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on))!=0)
	{
		printf("stunnel reuse addr error!,file: %s,line:%d\n",__FILE__,__LINE__);
	}

	memset(&serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	if (bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(struct sockaddr)) < 0) 
	{
		shutdown(sockfd, SHUT_RDWR);
		printf("stunnel fail to bind socket to client!\n");
	}

	/* 监听该端口,若失败则切断进程共享的套接字的读和写 */
	if (listen(sockfd, LISTEN_BACKLOG) < 0) 
	{
		shutdown(sockfd, SHUT_RDWR);
		printf("stunnel fail to listen socket to client!\n");
	}
	printf("start listen port:%d...\n",port);
	return sockfd;
}

int stunnel_get_client_fd(int stl_socket_fd,int port) 
{
	int									client_fd;
	struct sockaddr_in					client_addr;

	socklen_t client_size = sizeof(struct sockaddr);
	memset(&client_addr, 0, client_size);

	client_fd = accept(stl_socket_fd, (struct sockaddr *) &client_addr, &client_size);
	if (client_fd < 0)		
	{
		return -1;
		printf("heer?,%d\n",__LINE__);
	}
	printf("client [ip:port] is:[%s:%u]\n",inet_ntoa(client_addr.sin_addr),port);

	return client_fd;
}


SSL_CTX * SSL_init(str_stunnel *ip_port_stunnel)
{
	SSL_CTX			*ctx;

	/* SSL lib init*/
	SSL_library_init();

	/* load all SSL algorithms */
	OpenSSL_add_all_algorithms();

	/* load all SSL error info */
	SSL_load_error_strings();

	/* 以SSL V2 和V3 标准兼容方式产生一个SSL_CTX ，即SSL Content Text */
	ctx = SSL_CTX_new(SSLv23_client_method());
	if (ctx == NULL) 
	{
		printf("at line %d CTX_new error!\n",__LINE__);
		ERR_print_errors_fp(stdout);
		exit(1);
	}

	/*通过CA对服务器端的身份做验证 */
	SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
	if(!SSL_CTX_load_verify_locations(ctx,ip_port_stunnel->CAfile, NULL))
	{
		printf("SSL_CTX_load_verify_locations error!\n");
		ERR_print_errors_fp(stderr);
		exit(1);
	}

	printf("CA file is:%s\n",ip_port_stunnel->CAfile);
	return ctx;
}


void ShowCerts(SSL * ssl) 
{
	X509 * cert;
	char * line;
	cert = SSL_get_peer_certificate(ssl);
	if (cert != NULL) 
	{
		printf("数字证书信息:\n");
		line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
		printf("证书: %s\n", line);
		free(line);
		line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
		printf("颁发者: %s\n", line);
		free(line);
		X509_free(cert);
	} 
	else 
	{
		printf("无证书信息！\n");
	}
}


int stunnel_socket_init_to_server(char *ip,int port)
{
	int									server_fd=0;
	struct sockaddr_in                  dest;

	/*create socket*/
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{
		perror("Socket");
		exit(errno);
	}
	printf("stunnel created socket to server!\n");
	printf("ip is:%s,port is:%d\n",ip,port);

	/* init port 、IP*/
	bzero( &dest, sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_port = htons(port);
	if (inet_aton(ip,(struct in_addr *)&dest.sin_addr.s_addr) == 0) 
	{
		perror(ip);
		exit(errno);
	}

	/* connect to server */
	if (connect(server_fd, (struct sockaddr * ) &dest, sizeof(dest)) < 0) 
	{
		perror("Connect ");
		exit(errno);
	}
	printf("stunnel success connected server!\n");
	return server_fd;
}


int conf_parser(char *path,int max,str_stunnel *ip_port_stunnel)
{
	dictionary						*ini= NULL;
	char							key[64];
	int								i=0;
	char						    *str_ip=NULL;
	char							*str_ip_point=NULL;
	int								client_port;
	char							*str_CA=NULL;

	ini = iniparser_load(path);
	if( ini ==NULL)
	{
		printf("load conf file failure!\n");
		return -1;
	}

	for(i=1;i<=max;i++)
	{
		/*parser server port 、server ip */
		snprintf(key,sizeof(key),"stunnel%d:connect",i);
		str_ip=iniparser_getstring(ini,key, "null");
		str_ip_point=strstr(str_ip,":");



		/*parser client port */
		snprintf(key,sizeof(key),"stunnel%d:accept",i);
		client_port=iniparser_getint(ini,key,3);

		/*parser CAfile*/
		snprintf(key,sizeof(key),"stunnel%d:CAfile",i);
		str_CA=iniparser_getstring(ini,key, "null");

		if( str_ip&&client_port )
		{
			printf("stunnel%d parser is ok!\n",i);
			printf("tunnel%d:[%d====>%s]\n",i,client_port,str_ip);
			printf("CAfile is :%s\n", str_CA);
		}
		else
			return -1;

		/*load data to struct ip_port_stunnel[i] */
		ip_port_stunnel[i].server_port =atoi(str_ip_point+1);	
		ip_port_stunnel[i].server_ip = strtok(str_ip,":");
		ip_port_stunnel[i].client_port = client_port;
		ip_port_stunnel[i].CAfile=str_CA;
	}
}


/*
 * 1.create thread 
 * 2.waiting socket connect
 * */
int create_thread_to_listen(str_stunnel *ip_port_stunnel)
{
	pthread_attr_t						 thread_attr;
	pthread_t							 tid;

	/*init thread attribution "thread_attr"*/
	if( pthread_attr_init(&thread_attr) )
	{
		printf("pthread_attr_init() failure: %s\n", strerror(errno));
		goto CleanUp;
	}

	/*set thread stack size */
	if( pthread_attr_setstacksize(&thread_attr, 120*1024) )
	{
		printf("pthrread_attr_setstacksize() failure: %s\n", strerror(errno));
		goto CleanUp;
	}

	/*set thread attribution to detached PTHREAD_CREATE_DETACHED*/
	if( pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED) )
	{
		printf("pthread_attr_setdetachstate() failure: %s\n", strerror(errno));
		goto CleanUp;
	}

	/*create child thread */
	pthread_create(&tid,&thread_attr,work_func,ip_port_stunnel);					
	while(1);
CleanUp:
	return 0;
}


void* work_func(void* arg)
{
	char								buf[1024];
	int									acpt_fd;
	int									rv=-1;
	str_stunnel							*ip_port_stunnel;
	int									socket_fd=0;
	int									client_fd=0;
	int									epoll_rt=0;
	int									epollfd;

	ip_port_stunnel=arg;

	/*establish a socket connection with client */
	socket_fd =stunnel_socket_init_to_client(ip_port_stunnel->client_port);
	printf("at line:%d,socket_fd is:%d\n",__LINE__,socket_fd);

	epollfd=epoll_init(socket_fd);
	if(epollfd<0)
	{
		printf("epoll_create() failure: %s\n", strerror(errno));
	}
	while(1)
	{
		/*add socket_fd to epoll prevent lose event */
		epoll_rt=epoll_to_listen_events(epollfd,socket_fd,ip_port_stunnel);
		if(epoll_rt<0)
			printf("you will did\n");
	}
}

int epoll_init(int socket_fd)
{

	int									epollfd;
	struct epoll_event					event;
	struct epoll_event					event_array[MAX_EVENTS];
	int									events;

	/*创建epoll对象实例，size指定要通过epoll实例来检查的文件描述符个数*/
	if( (epollfd=epoll_create(MAX_EVENTS)) < 0 )
	{
		printf("epoll_create() failure: %s\n", strerror(errno));
		return -1;
	}

	event.events = EPOLLIN;//可读取非高优先级数据
	event.data.fd = socket_fd;

	/* 并指定listenfd文件描述符上我们感兴趣的事件(event.events=EPOLLIN)*/
	if( epoll_ctl(epollfd, EPOLL_CTL_ADD,socket_fd, &event) < 0)
	{
		printf("epoll add listen socket failure: %s\n", strerror(errno));
		return -2;
	}
	return epollfd;
}

int epoll_to_listen_events(int epoll_fd,int socket_fd,str_stunnel *ip_port_stunnel)
{
	int									listenfd=0; 
	int									client_fd=0;
	int									server_fd=0;
	int									rv=0;
	int									i, j;
	char								buf[1024];

	int									epollfd;
	struct epoll_event					event;
	struct epoll_event					event_array[MAX_EVENTS];
	int									events;
	SSL									*ssl=NULL;
	stunnel_map							stl_map[MAX_EVENTS];
	int									trans_rt=0;

	/* blocking here ,through event_arrary return fd info */
	printf("I'm waiting client to connect...\n");
	events = epoll_wait(epoll_fd, event_array, MAX_EVENTS, -1);
	if(events < 0)
	{
		printf("epoll failure: %s\n", strerror(errno));
		exit(1);
	}
	else if(events == 0)
	{
		printf("epoll get timeout\n");
		exit(0);
	}


	/* rv>0 has active events */
	for(i=0; i<events; i++)
	{
		/* error or hang up */
		if ( (event_array[i].events&EPOLLERR) || (event_array[i].events&EPOLLHUP) )
		{
			printf("epoll_wait get error on fd[%d]: %s\n", event_array[i].data.fd, strerror(errno));
			epoll_ctl(epollfd, EPOLL_CTL_DEL, event_array[i].data.fd, NULL);
			close(event_array[i].data.fd);
		}

		/* new client is coming */
		if( event_array[i].data.fd == socket_fd )
		{ 
			/*accept client */
			client_fd=stunnel_get_client_fd(socket_fd,ip_port_stunnel->client_port);

			event.events =  EPOLLIN|EPOLLET;
			event.data.fd = client_fd;
			if( epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) < 0 )
			{
				printf("epoll add client socket failure: %s\n", strerror(errno));
				close(event_array[i].data.fd);//关闭就绪态文件描述符
				continue;
			}
			printf("epoll add new client_fd is:%d \n", client_fd);


			/*create ssl to server */
			server_fd= stunnel_socket_init_to_server(ip_port_stunnel->server_ip,ip_port_stunnel->server_port);

			event.data.fd = server_fd;
			event.events =  EPOLLIN|EPOLLET;//可读取非优先级数据、边沿触发
			if( epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) < 0 )
			{
				printf("epoll add server socket failure: %s\n", strerror(errno));
				close(event_array[i].data.fd);//关闭就绪态文件描述符
				continue;
			}
			else
			{
				printf("at line:%d\n",__LINE__);
				stl_map[count].client_fd= client_fd;
				stl_map[count].server_fd= server_fd;
				ssl=stunnel_ssl_init_to_server(&stl_map[count],ip_port_stunnel);
				stl_map[count].ssl=ssl;
				count++;
				printf("ssl init ok ! at line:%d\n",__LINE__);
			}

		}		
		else
		{
			trans_rt=transmit(stl_map,&event_array[i],events);
			if(trans_rt<0)
				printf("at line %d transmit failture!\n",__LINE__);		
		}
	}

	return 0;

CleanUp:
	close(listenfd);
	return 0;
}

SSL * stunnel_ssl_init_to_server(stunnel_map *stl_map,str_stunnel *ip_port_stunnel)
{
	SSL									*ssl=NULL;
	SSL_CTX								*ctx;

	/*ssl init*/
	ctx=SSL_init(ip_port_stunnel);

	/* base on ctx ,create a new SSL */
	ssl = SSL_new(ctx);

	/* add socket fd to SSL */
	SSL_set_fd(ssl,stl_map->server_fd);

	/* establish SSL connection */
	if (SSL_connect(ssl) == -1) 
	{
		printf("at line %d SSL_connect error!\n",__LINE__);
		ERR_print_errors_fp(stderr);
	}	
	else 
	{
		printf("Connected with %s encryption\n", SSL_get_cipher(ssl));
		ShowCerts(ssl);
	}

	return ssl;
}

int transmit(stunnel_map *stl_map,struct epoll_event *event_array,int events)
{
	int									i=0;
	int									j=0;	
	char								buffer[128];
	int								    len=0;
	int								    write_rt=0;
	int									read_rt=0;

	bzero(buffer, sizeof(buffer));
	printf("at line %d\n",__LINE__);
	/*后面的读写操作必须根据多路复用来实现 */
	for(i=0;i<count;i++)
	{
		/* receive data*/
		if(event_array->data.fd == stl_map[i].server_fd)
		{
			printf("found ok!\n");
			len = SSL_read(stl_map[i].ssl, buffer, sizeof(buffer));
			if (len > 0) 
			{
				printf("stunnel success receive %d bytes from server content is:'%s'\n", len,buffer);
				write_rt=write(stl_map[i].client_fd, buffer, sizeof(buffer));
				printf("at line %d client_fd is %d\n",__LINE__,stl_map[i].client_fd);
				if(write_rt>0)
				{
					printf("stunnel write to client success!\n");
					return 0;
				}
				else
				{
					printf("stunnel write client failture!\n");
					return -1;
				}
			}
			else 
			{
				printf("stunnel SSL_read failture! error info'%s'\n", strerror(errno));
				return -2;	
			}
		}


		/*send message*/
		if(event_array->data.fd == stl_map[i].client_fd)
		{
			bzero(buffer, sizeof(buffer));
			printf("at line %d client_fd is %d\n",__LINE__,stl_map[i].client_fd);
			len = read(stl_map[i].client_fd, buffer, sizeof(buffer));
			printf("at line %d stunnel read data is :%s\n",__LINE__,buffer);

			if (len < 0) 
			{
				printf("read failture! error info:%s\n",strerror(errno));			
				return -3;
			}
			else 
			{
				/*this section is ok!*/
				printf("stunnel success read %d bytes from client! \n",len);
				//memcpy(buffer,"I'm stunnel!",20);
				printf("SSL start write!\n");
				len=SSL_write(stl_map[i].ssl,buffer,sizeof(buffer));
				if(len>0)
				{
					printf("stunnel success ssl_write\n");
					return 0;
				}
				else
				{
					printf("SSL_write failture!\n");
					return -4;
				}
			}
		}
		else
		{
			printf("event not happen!\n");
			return -1;
		}
	}
}
