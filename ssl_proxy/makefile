VAR= stunnel.o dictionary.o iniparser.o
test:$(VAR)
	gcc -o test $(VAR) -lcrypto -lssl -lpthread
%.o:%.c
	gcc -c $< -o $@
.PHONY:clean
clean:
	                rm -rf *.o a.out test
	
