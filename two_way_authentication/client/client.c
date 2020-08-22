#include <stdio.h> 
#include <string.h> 
#include <errno.h> 
#include <sys/socket.h> 
#include <resolv.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <openssl/ssl.h> 
#include <openssl/err.h> 

#include "getopt_client.h"
#define MAXBUF 1024 

SSL_CTX * SSL_init(opt_arg opt_rt);
int socket_init(opt_arg ip_port_st);
void ShowCerts(SSL * ssl);

int main(int argc, char **argv) 
{
	int									sockfd, len;
	struct sockaddr_in							dest;
	char									buffer[MAXBUF + 1];
	SSL_CTX									*ctx;
	SSL									*ssl;
	opt_arg									ip_port_st;

	/*get ip、port*/
	ip_port_st=getopt_client(argc,argv);

	/* SSL init*/
	ctx=SSL_init(ip_port_st);	

	/* socket init*/
	sockfd=socket_init(ip_port_st);	

	/* 基于ctx 产生一个新的SSL */
	ssl = SSL_new(ctx);

	/* 将新连接的socket 加入到SSL */
	SSL_set_fd(ssl, sockfd);

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

finish:
	/* close connect*/
	SSL_shutdown(ssl);
	SSL_free(ssl);
	close(sockfd);
	SSL_CTX_free(ctx);
	return 0;
}

SSL_CTX * SSL_init(opt_arg opt_rt)
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
		ERR_print_errors_fp(stdout);
		return -1;
	}

	/* 载入client的数字证书， 此证书用来发送给server端。证书里包含有公钥 */
	if (SSL_CTX_use_certificate_file(ctx,opt_rt.cert, SSL_FILETYPE_PEM) <= 0) 
	{
		ERR_print_errors_fp(stdout);
		exit(1);
	}

	/* 载入client用户私钥 */
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

int socket_init(opt_arg ip_port_st)
{
	int									sockfd=0;
	struct sockaddr_in                  					dest;

	/* 创建一个socket 用于tcp 通信 */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{
		perror("Socket");
		exit(errno);
	}
	printf("socket created\n");
	printf("ip is:%s,port is:%d\n",ip_port_st.ip,ip_port_st.port);

	/* 初始化服务器端（对方）的地址和端口信息 */
	bzero( &dest, sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_port = htons(ip_port_st.port);
	if (inet_aton(ip_port_st.ip,(struct in_addr *)&dest.sin_addr.s_addr) == 0) 
	{
		perror(ip_port_st.ip);
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
