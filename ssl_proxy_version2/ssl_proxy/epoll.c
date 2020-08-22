#include "epoll.h"
#include "socket.h"
#include "parser.h"
#include <sys/epoll.h>

int epoll_init(int socket_fd)
{
	int									epollfd;
	struct epoll_event							event;
	struct epoll_event							event_array[MAX_EVENTS];
	int									events;

	/*创建epoll对象实例，size指定要通过epoll实例来检查的文件描述符个数*/
	if( (epollfd=epoll_create(MAX_EVENTS)) < 0 )
	{
		printf("epoll_create() failure: %s\n", strerror(errno));
		exit(1);
	}

	event.events = EPOLLIN;//可读取非高优先级数据
	event.data.fd = socket_fd;

	/* 并指定listenfd文件描述符上我们感兴趣的事件(event.events=EPOLLIN)*/
	if( epoll_ctl(epollfd, EPOLL_CTL_ADD,socket_fd, &event) < 0)
	{
		printf("epoll add listen socket failure: %s\n", strerror(errno));
		close(socket_fd);
		exit(1);
	}
	return epollfd;
}

int epoll_to_listen_events(int epoll_fd,int socket_fd,stunnel_t *stunnel)
{
	int									client_fd=-1;
	int									server_fd=-1;
	int									i=0;
	char									buf[1024];

	int									epollfd;
	struct epoll_event							event;
	struct epoll_event							event_array[MAX_EVENTS];
	int									events;
	SSL									*client_ssl=NULL;
	SSL                                 					*server_ssl=NULL;
	stunnel_map								stl_map[MAX_EVENTS];
	int									trans_rt=0;
	SSL_CTX *								ctx;
	int									ret=0;

	if(!stunnel)
	{
		printf("invalid formal argument 'stunnel_t *stunnel'\n");
		exit(1);
	}


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
			/*load self-signed cert\private key and use CAfile verify client*/
			ctx=SSL_init_client(stunnel->client_CAfile,stunnel);

			/*accept client */
			client_fd=socket_accept_client(socket_fd,stunnel->client_port);
			if(client_fd<0)
			{
				printf("socket_accept_client is failure!\n");
				close(client_fd);
				continue;
			}
			/* base on ctx craete a new SSL */
			client_ssl = SSL_new(ctx);
			if(!client_ssl)
			{
				SSL_shutdown(client_ssl);
				SSL_free(client_ssl);
				close(client_fd);
				continue;
			}
			/* add socket fd to SSL */
			SSL_set_fd(client_ssl, client_fd);

			printf("stunnel start SSL_accept...\n");
			/* establish SSL connection */
			if (SSL_accept(client_ssl)<0)
			{
				printf("stunnel SSL_accept client failure!\n");
				perror("SSL_accept");
				SSL_shutdown(client_ssl);
				SSL_free(client_ssl);
				close(client_fd);
				continue;
			}

			/*ensure TLS/SSL、I/O operate accomplish*/	
			SSL_get_error(client_ssl,ret);
			if(ret==SSL_ERROR_NONE )
			{
				printf("Connected with %s encryption\n", SSL_get_cipher(client_ssl));
				ShowCerts(client_ssl);
			}
			
			/*epoll add events*/
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
			server_fd= socket_connect_server(stunnel->server_ip,stunnel->server_port);
			if(server_fd<0)
			{
				close(server_fd);
				SSL_shutdown(client_ssl);
				SSL_free(client_ssl);
				close(client_fd);
				continue;
			}
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
				printf("in file %s at line:%d\n",__FILE__,__LINE__);
				server_ssl=socket_bind_ssl(server_fd,stunnel);
				if(!server_ssl)
				{
					SSL_shutdown(server_ssl);
					SSL_free(server_ssl);
					close(server_fd);
					SSL_shutdown(client_ssl);
					SSL_free(client_ssl);
					close(client_fd);
				}
				stl_map[count].client_fd=client_fd;
				stl_map[count].server_fd=server_fd;
				stl_map[count].client_ssl= client_ssl;
				stl_map[count].server_ssl= server_ssl;
				count++;
				printf("ssl init ok ! at line:%d\n",__LINE__);
			}
		}		
		else
		{
			/*find map*/
			for(i=0;i<count;i++)
			{
				/* receive data*/
				if(event_array->data.fd == stl_map[i].server_fd)
				{
					trans_rt=from_server_forward_to_client(stl_map);
					if(trans_rt<0)
					{
						printf("at line %d socket_forward failture!\n",__LINE__);
						SSL_shutdown(server_ssl);
						SSL_free(server_ssl);
						close(server_fd);
						SSL_shutdown(client_ssl);
						SSL_free(client_ssl);
						close(client_fd);
						continue;
					}
				}

				/*send data*/
				if(event_array->data.fd == stl_map[i].client_fd)
				{
					trans_rt=from_client_forward_to_server(stl_map);
					if(trans_rt<0)
					{
						printf("at line %d socket_forward failture!\n",__LINE__);
						SSL_shutdown(server_ssl);
						SSL_free(server_ssl);
						close(server_fd);
						SSL_shutdown(client_ssl);
						SSL_free(client_ssl);
						close(client_fd);
						continue;
					}
				}
			}
		}
	}

	return 0;
}


