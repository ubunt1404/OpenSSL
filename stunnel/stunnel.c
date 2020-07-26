#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/param.h>
#include <linux/netfilter_ipv4.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <unistd.h>
#include <getopt.h>

#include <openssl/ssl.h> 
#include <openssl/err.h> 

#define LISTEN_BACKLOG 13

int getopt_client(int argc,char **argv);
int stunnel_socket_init_to_client(int port);
int stunnel_get_client_fd(int stl_socket_fd,int port);
SSL_CTX * SSL_init();
void ShowCerts(SSL * ssl);
int stunnel_socket_init_to_server(char *ip,int port);

int main(int argc,char *argv[])
{
	short int							port_cli=0;
	short int                           port_sev=0;
	int									socket_fd_cli=0;
	int                                 socket_fd_sev=0;
	struct sockaddr_in                  client_dest_addr;
	int								    client_fd=0;
	SSL_CTX *							ctx;
	int									len=0;
	struct sockaddr_in					dest;
	char								buffer[128];
	SSL									*ssl;
	char								*ip="127.0.0.1";

	port_cli=getopt_client(argc,argv);

	/*ssl init*/
	ctx=SSL_init();

	/*初始化一个socket,并监听client端来连接的端口 */
	socket_fd_cli =stunnel_socket_init_to_client(port_cli);

	while (1)
	{
		client_fd=stunnel_get_client_fd(socket_fd_cli,port_cli);
		if (client_fd < 0)
		{
			printf("stunnel accept client error,get client_fd failture!\n");
			return -1;
		}
		printf("success get client fd!\n");

		
		socket_fd_sev=stunnel_socket_init_to_server(ip,port_sev);
		/* 基于ctx 产生一个新的SSL */
		ssl = SSL_new(ctx);

		/* 将新连接的socket 加入到SSL */
		SSL_set_fd(ssl,socket_fd_sev);

		/* 建立SSL 连接*/
		if (SSL_connect(ssl) == -1) 
		{
			ERR_print_errors_fp(stderr);
		}
		else 
		{
			printf("Connected with %s encryption\n", SSL_get_cipher(ssl));
			ShowCerts(ssl);
		}

		bzero(buffer, sizeof(buffer));
		/* receive data*/
		len = SSL_read(ssl, buffer, sizeof(buffer));
		if (len > 0) 
		{
			printf("client success receive %d bytes from server content is:'%s'\n", len,buffer);
		}
		else 
		{
			printf("read failture! errno code:%d error info'%s'\n", errno, strerror(errno));
			goto finish;
		}
		bzero(buffer, sizeof(buffer));
		strcpy(buffer, "client success receive!");

		/*send message*/
		len = SSL_write(ssl, buffer, strlen(buffer));
		if (len < 0) 
		{
			printf("write failture! errno code:%d，error info:%s'\n",errno, strerror(errno));
		}
		else 
		{
			printf("client success send %d bytes! \n",len);
		}
	}
finish:
	/* close connect*/
	SSL_shutdown(ssl);
	SSL_free(ssl);
	close(socket_fd_cli);
	SSL_CTX_free(ctx);
	return 0;
}


int getopt_client(int argc,char **argv)
{
	unsigned short					 port=0;
	int								 c=0;

	const struct option longopts[]=
	{
		{"port",  required_argument,0,'p'},
		{"help",  no_argument      ,0,'h'},
		{NULL,0,NULL,0}
	};	
	while((c=getopt_long(argc,argv,"p:h",longopts,NULL))!=-1)
	{
		/*c必须放在while循环内*/
		switch(c)
		{
			case 'p':
				port=atoi(optarg);
				break;
			case 'h':
				printf("端口：--p  [args] \nIP地址：--i [args]\n帮助 h\n");
				exit(0);
		}
	}
	if(!port)
	{
		printf("请输入port！\n");
		exit(0);
	}
	return port;
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
		printf("heer?\n");
	}
	printf("client [ip:port] is:[%s:%u]\n",inet_ntoa(client_addr.sin_addr),port);

	return client_fd;
}

SSL_CTX * SSL_init()
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
		ERR_print_errors_fp(stdout);
		exit(1);
	}

	/*通过CA对服务器端的身份做验证 */
	SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
	if(!SSL_CTX_load_verify_locations(ctx,"ca.crt", NULL))
	{
		printf("SSL_CTX_load_verify_locations error!\n");
		ERR_print_errors_fp(stderr);
		exit(1);
	}
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
	int									sockfd=0;
	struct sockaddr_in                  dest;

	/* 创建一个socket 用于tcp 通信 */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{
		perror("Socket");
		exit(errno);
	}
	printf("stunnel created socket to server!\n");
	printf("ip is:%s,port is:%d\n",ip,port);

	/* 初始化服务器端（对方）的地址和端口信息 */
	bzero( &dest, sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_port = htons(port);
	if (inet_aton(ip,(struct in_addr *)&dest.sin_addr.s_addr) == 0) 
	{
		perror(ip);
		exit(errno);
	}
	printf("address created\n");

	/* 连接服务器 */
	if (connect(sockfd, (struct sockaddr * ) &dest, sizeof(dest)) != 0) 
	{
		perror("Connect ");
		exit(errno);
	}
	printf("server connected\n");
	return sockfd;
}

