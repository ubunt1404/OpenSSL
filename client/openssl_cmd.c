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

/* 接收对方发过来的消息，最多接收MAXBUF 个字节*/
bzero(buffer, MAXBUF + 1);

/* 接收服务器来的消息*/
len = SSL_read(ssl, buffer, MAXBUF);
if (len > 0) 
{
	printf("接收消息成功:'%s'，共%d 个字节的数据\n", buffer, len);
}
else 
{
	printf("消息接收失败！错误代码是%d，错误信息是'%s'\n", errno, strerror(errno));
	goto finish;
}
bzero(buffer, MAXBUF + 1);
strcpy(buffer, "from client->server");

/* 发消息给服务器*/
len = SSL_write(ssl, buffer, strlen(buffer));
if (len < 0) 
{
	printf("消息'%s'发送失败！错误代码是%d，错误信息是'%s'\n", buffer, errno, strerror(errno));
}
else 
{
	printf("消息'%s'发送成功，共发送了%d 个字节！\n", buffer, len);
}

finish:
/* 关闭连接*/
SSL_shutdown(ssl);
SSL_free(ssl);
close(sockfd);
SSL_CTX_free(ctx);
return 0;
