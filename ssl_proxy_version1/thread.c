#include "parser.h"
#include "socket.h"
#include "thread.h"



/*
 * 1.create thread 
 * 2.waiting socket connect
 * */
int									g_sigstop;
int create_thread_to_listen(stunnel_t *stunnel)
{
	pthread_attr_t						thread_attr;
	pthread_t							tid;
	
	if(!stunnel)
	{
		printf("invalid formal argument 'stunnel_t *stunnel'\n");
		exit(1);
	}
	
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

	/*set thread attribution to detached PTHREAD_CREATE_DETACHED 、PTHREAD_CREATE_JOINABLE*/
	if( pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED) )
	{
		printf("pthread_attr_setdetachstate() failure: %s\n", strerror(errno));
		goto CleanUp;
	}

	/*create child thread */
	pthread_create(&tid,&thread_attr,work_stunnel,stunnel);

	while(1);//use to test 
CleanUp:
	pthread_attr_destroy(&thread_attr);
	return -1;
}

/*statement:when a thread exits the opened file descriptors do not actively(主动) release */
void* work_stunnel(void* arg)
{
	char								buf[1024];
	int									acpt_fd;
	int									rv=-1;
	stunnel_t							*stunnel;
	int									socket_fd=0;
	int									client_fd=0;
	int									epoll_rt=0;
	int									epollfd;

	stunnel=arg;
	
	/*install signale */
	signal(SIGINT, signal_stop);
	
	/*used for establish a socket connection with client */
	socket_fd =socket_init_listen(stunnel->client_port);
	printf("at line:%d,socket_fd is:%d\n",__LINE__,socket_fd);

	/*epoll init*/
	epollfd=epoll_init(socket_fd);
	
	while(!g_sigstop)
	{
		/*add socket_fd to epoll prevent lose event */
		epoll_rt=epoll_to_listen_events(epollfd,socket_fd,stunnel);
		if(epoll_rt<0)
			printf("you will did\n");
	}
}


void signal_stop(int signum)
{
	if(signum == SIGINT)
	{
		//printf("stunnel will eixt!\n");
		g_sigstop = 1;
		exit(0);
	}
}
