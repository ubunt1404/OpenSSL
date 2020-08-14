#include "socket.h"
#include "epoll.h"
#include "parser.h"
int				count;
int socket_init_listen(int port) 
{
	int									sockfd=0;
	int									on = 1;
	struct sockaddr_in					serv_addr;
	struct sockaddr_in                  client_dest_addr;
	int								    client_fd=0;
	
	if(port<=0)
	{
		printf("invalid formal argument 'int port'\n");
		exit(1);
	}
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("stunnel fail to create socket !\n");
		exit(1);
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
		close(sockfd);
		printf("stunnel fail to bind socket to client!\n");
		exit(1);
	}

	if (listen(sockfd, LISTEN_BACKLOG) < 0) 
	{
		close(sockfd);
		printf("stunnel fail to listen socket to client!\n");
		exit(1);
	}
	printf("start listen port:%d...\n",port);
	return sockfd;
}


int socket_accept_client(int stl_socket_fd,int port) 
{
	int									client_fd;
	struct sockaddr_in					client_addr;

	socklen_t client_size = sizeof(struct sockaddr);
	memset(&client_addr, 0, client_size);

	client_fd = accept(stl_socket_fd, (struct sockaddr *) &client_addr, &client_size);
	if (client_fd < 0)		
	{
		printf("stunnel accept client connection failure!\n");
		exit(1);
	}
	printf("client [ip:port] is:[%s:%u]\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));

	return client_fd;
}




int socket_connect_server(char *ip,int port)
{
	int									server_fd=0;
	struct sockaddr_in                  dest;

	/*create socket*/
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{
		printf("stunnel create socket to server failure!\n");
		exit(1);
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



SSL * socket_bind_ssl(stunnel_map *stl_map,stunnel_t *stunnel)
{
	SSL									*ssl=NULL;
	SSL_CTX								*ctx;

	if(!stl_map)
	{
		printf("invalid formal argument 'stunnel_map *stl_map'\n");
		exit(1);
	}

	/*ssl init*/
	ctx=SSL_init(stunnel);

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


int socket_forward(stunnel_map *stl_map,struct epoll_event *event_array)
{
	int									i=0;
	int									j=0;	
	char								buffer[128];
	int								    len=0;
	int								    write_rt=0;
	int									read_rt=0;

	if(!stl_map || !event_array)
	{
		printf("invalid formal argument\n");
		exit(1);
	}

	bzero(buffer, sizeof(buffer));
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
	}
}


SSL_CTX * SSL_init(stunnel_t *stunnel)
{
	SSL_CTX								*ctx;

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
	if(!SSL_CTX_load_verify_locations(ctx,stunnel->CAfile, NULL))
	{
		printf("SSL_CTX_load_verify_locations error!\n");
		ERR_print_errors_fp(stderr);
		exit(1);
	}

	printf("CA file is:%s\n",stunnel->CAfile);
	return ctx;
}


void ShowCerts(SSL * ssl) 
{
	X509								* cert;
	char								* line;
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


