VAR= stunnel.o dictionary.o iniparser.o parser.o socket.o thread.o epoll.o
test:$(VAR)
	gcc -o test $(VAR) -lcrypto -lssl -lpthread
%.o:%.c
	gcc -c $< -o $@
.PHONY:clean
clean:
	                rm -rf *.o a.out test
	
