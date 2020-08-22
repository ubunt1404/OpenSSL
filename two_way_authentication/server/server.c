#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <sys/wait.h> 
#include <unistd.h> 
#include <arpa/inet.h> 
#include <openssl/ssl.h> 
#include <openssl/err.h> 
#include "getopt_server.h"

#define MAXBUF 1024 

SSL_CTX *SSL_init(opt_arg opt_rt);
int socket_init(opt_arg opt_rt);
void ShowCerts(SSL * ssl);

int main(int argc, char * *argv) 
{
	int								sockfd, new_fd;
	socklen_t							len;
	struct sockaddr_in						my_addr, their_addr;
	char								buf[MAXBUF + 1];
	SSL_CTX								*ctx;
	opt_arg								opt_rt;
	SSL								*ssl;

	/*get key、certification、port*/
	opt_rt=getopt_server(argc,argv);

	/*ssl init*/
	ctx=SSL_init(opt_rt);

	/*socket init*/
	sockfd=socket_init(opt_rt);

	while (1) 
	{
		len = sizeof(struct sockaddr);

		/*wating for connect...*/
		if ((new_fd = accept(sockfd, (struct sockaddr * ) & their_addr, &len)) == -1) 
		{
			perror("accept");
			exit(errno);
		} 
		else 
		{
			printf("server: got connection from %s, port %d, socket %d\n", inet_ntoa(their_addr.sin_addr), ntohs(their_addr.sin_port), new_fd);
		}

		/* 基于ctx 产生一个新的SSL */
		ssl = SSL_new(ctx);

		/* 将连接用户的socket 加入到SSL */
		SSL_set_fd(ssl, new_fd);

		/* 建立SSL 连接*/
		if (SSL_accept(ssl) == -1) 
		{
			perror("accept");
			close(new_fd);
			break;
		}
		else
		{
			printf("Connected with %s encryption\n", SSL_get_cipher(ssl));
			ShowCerts(ssl);
		}
		/* 开始处理每个新连接上的数据收发*/
		bzero(buf, MAXBUF + 1);
		strcpy(buf, "server success receive!");
		/*send message to client!*/
		len = SSL_write(ssl, buf, strlen(buf));
		if (len <= 0) 
		{
			printf("send failture! errno code:%d error info:%s\n",errno, strerror(errno));
			goto finish;
		} 
		else 
		{
			printf("server success send %d bytes to client!\n",len);
		}

		bzero(buf,sizeof(buf));
		/*receive message from client!*/
		len = SSL_read(ssl, buf, MAXBUF);
		if (len > 0) 
		{
			printf("server success receive %d bytes from client! content is:%s\n",len,buf);
		}
		else 
		{
			printf("send failture! errno code:%d error info:%s\n", errno, strerror(errno));
		}

		/*close connect*/
finish:
		/* close SSL connect*/
		SSL_shutdown(ssl);
		/* free SSL */
		SSL_free(ssl);
		/* close socket */
		close(new_fd);
	}

	/* close listen socket */
	close(sockfd);
	/*free CTX */
	SSL_CTX_free(ctx);
	return 0;
}

void ShowCerts(SSL * ssl)
{
	X509 *cert;
	char *line;

	cert = SSL_get_peer_certificate(ssl);
	if(SSL_get_verify_result(ssl) == X509_V_OK)
	{
		printf("证书验证通过\n");
	}
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
		printf("无证书信息！\n");
}
SSL_CTX *SSL_init(opt_arg opt_rt)
{
	SSL_CTX *					ctx;

	/* SSL lib init */
	SSL_library_init();
	/* load all SSL algorithms */
	OpenSSL_add_all_algorithms();
	/* load all SSL error info */
	SSL_load_error_strings();
	/* 以SSL V2 和V3 标准兼容方式产生一个SSL_CTX ，即SSL Content Text */
	ctx = SSL_CTX_new(SSLv23_server_method());
	/* 也可以用SSLv2_server_method() 或SSLv3_server_method() 单独表示V2 或V3标准*/
	if (ctx == NULL) 
	{
		ERR_print_errors_fp(stdout);
		exit(1);
	}

	/* 验证客户端的身份 */
	SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
	if (SSL_CTX_load_verify_locations(ctx, "ca.crt",NULL)<=0)
	{
		ERR_print_errors_fp(stdout);
		exit(1);
	}


	/* 载入用户的数字证书， 此证书用来发送给客户端。证书里包含有公钥*/
	if (SSL_CTX_use_certificate_file(ctx,opt_rt.cert, SSL_FILETYPE_PEM) <= 0) 
	{
		ERR_print_errors_fp(stdout);
		exit(1);
	}

	/* 载入用户私钥*/
	if (SSL_CTX_use_PrivateKey_file(ctx,opt_rt.key, SSL_FILETYPE_PEM) <= 0) 
	{
		ERR_print_errors_fp(stdout);
		exit(1);
	}

	/* 检查用户私钥是否正确*/
	if (!SSL_CTX_check_private_key(ctx)) 
	{
		ERR_print_errors_fp(stdout);
		exit(1);
	}
	return ctx;
}
int socket_init(opt_arg opt_rt)
{
	struct sockaddr_in					   my_addr;
	int							   sockfd =0;	
	/* 开启一个socket 监听*/
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) 
	{
		perror("socket");
		exit(1);
	} 
	else 
	{
		printf("socket created\n");
	}

	bzero( &my_addr, sizeof(my_addr));
	my_addr.sin_family = PF_INET;
	my_addr.sin_port = htons(opt_rt.port);
	my_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(sockfd, (struct sockaddr * ) &my_addr, sizeof(struct sockaddr)) == -1) 
	{
		perror("bind");
		exit(1);
	} 
	else
	{
		printf("binded\n");
	} 

	if (listen(sockfd, 13) == -1) 
	{
		perror("listen");
		exit(1);
	} 
	else 
	{
		printf("begin listen port:%d\n",opt_rt.port);
	}
	return sockfd;
}
