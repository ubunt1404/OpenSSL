VARABLE= server.o  getopt_server.o 
test:$(VARABLE)
		gcc -o test $(VARABLE) -lcrypto -lssl
%.o:%.c
		gcc -c $< -o $@ 
		#$<=所有c文件 $@=所有目标文件
.PHONY:clean
clean:
		rm -rf *.o a.out test ca.* server.crt server.csr server.key demoCA
