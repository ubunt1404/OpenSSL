#include "epoll.h"
#include "socket.h"
#include "parser.h"
#include <sys/epoll.h>

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
			/*accept client */
			client_fd=socket_accept_client(socket_fd,stunnel->client_port);
			if(client_fd<0)
			{
				close(client_fd);
				continue;
			}
			event.events =  EPOLLIN|EPOLLET;
			event.data.fd = client_fd;
			if( epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) < 0 )
			{
				printf("epoll add client socket failure: %s\n", strerror(errno));
				close(event_array[i].data.fd);//关闭就绪态文件描述符
				continue;
			}
			printf("epoll add new client_fd is:%d \n", client_fd);


			/*create socket to connect server */
			server_fd= socket_connect_server(stunnel->server_ip,stunnel->server_port);
			if(server_fd<0)
			{
				close(client_fd);
				close(server_fd);
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
				printf("at line:%d\n",__LINE__);
				stl_map[count].client_fd= client_fd;
				stl_map[count].server_fd= server_fd;

				/*create ssl connection to server*/
				ssl=socket_bind_ssl(&stl_map[count],stunnel);
				if(!ssl)
				{
					goto finish;
				}
				stl_map[count].ssl=ssl;
				count++;
				printf("ssl init ok ! at line:%d\n",__LINE__);
			}
		}	

		else
		{	
			/*find map to forward data*/
			for(i=0;i<count;i++)
			{
				/* receive data*/
				if(event_array->data.fd == stl_map[i].server_fd)
				{
					trans_rt=from_server_forward_to_client(stl_map);
					if(trans_rt<0)
					{
						printf("at line %d socket_forward failture!\n",__LINE__);
						SSL_shutdown(stl_map[i].ssl);
						SSL_free(stl_map[i].ssl);
						close(stl_map[i].server_fd);
						close(stl_map[i].client_fd);
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
						close(stl_map[i].client_fd);
						SSL_shutdown(stl_map[i].ssl);
						SSL_free(stl_map[i].ssl);
						close(stl_map[i].server_fd);
						continue;
					}
				}
			}
		}
	}

	return 0;

finish:
	SSL_shutdown(ssl);
	SSL_free(ssl);
	close(server_fd);
	close(client_fd);
	return 0;
}


