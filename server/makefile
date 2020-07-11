VARABLE= socket.o  getopt_server.o socket_server_init.o 
test:$(VARABLE)
		gcc -o test $(VARABLE) 
%.o:%.c
		gcc -c $< -o $@
		#$<=所有c文件 $@=所有目标文件
.PHONY:clean
clean:
		rm -rf *.o a.out test 
