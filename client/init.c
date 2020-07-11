#include "init.h"
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <sys/types.h>  
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <netinet/in.h>

void SSL_init()
{
	SSL_CTX *			ctx;
	FILE *				ssl_error_out;

	/* SSL 库初始化*/
	SSL_library_init();

	/* 载入所有SSL 算法*/
	OpenSSL_add_all_algorithms();
	
	/* 载入所有SSL 错误消息*/
	SSL_load_error_strings();
	
	/*创建一个新的SSL_CTX对象作为启用TLS / SSL的功能的框架，以建立启用TLS / SSL的连接*/
	ctx = SSL_CTX_new(SSLv23_client_method());
	if (ctx == NULL) 
	{
		ERR_print_errors_fp(ssl_error_out);//将出错信息输出到ssl_error_out中
		exit(1);
	}
}

int socket_init(ip_port ip_port_opt)
{
	int                              sockfd=-1;
	int                              conect_rt=0;
	struct sockaddr_in               serv_addr;
	ip_port                          ip_port_st;
	
	/*socket指明大方面通信协议IPV4/6、小方面通信类型TCP/UDP*/
	sockfd =socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd<0)
	{
		printf("socket failure:%s\n",strerror(errno));
		return -1;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	/* int inet_aton(const char *cp, struct in_addr *inp);
	 * 将一个字符串IP地址转换为一个32位的网络序列IP地址
	 * inet_aton( SERVER_ADDR, &serv_addr.sin_addr );
	 */

	ip_port_st=ip_port_opt;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(ip_port_st.port);//对端口号进行处理
	inet_aton( ip_port_st.ip, &serv_addr.sin_addr );//对IP进行操作

	/*connect指定大的通信协议IPV4/6、通信端口、IP*/
	conect_rt =connect(sockfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr));
	printf("%d\n",conect_rt);
	if(conect_rt<0)
	{
		printf("connect failure:%s\n",strerror(errno));
		return -2;
	}
	else
		printf("connect successfully!\n");

	return sockfd;
}
