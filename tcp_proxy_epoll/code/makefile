src = $(wildcard ./*.c)
obj = $(patsubst %.c, %.o, $(src))
#obj = tcp_main.o tcp_conf.o tcp_proxy.o tcp_sock.o
target = tcp_proxy
CC = gcc

$(target): $(obj)
	$(CC) $(obj) -o $(target) -lpthread
%.o: %.c
	$(CC) -c $< -o $@
# tcp_proxy : tcp_main.o  tcp_conf.o tcp_proxy.o tcp_sock.o
# 	gcc tcp_main.o tcp_conf.o tcp_proxy.o tcp_sock.o -o tcp_proxy -lpthread
# tcp_main.o : tcp_main.c 
# 	gcc -c tcp_main.c 
# tcp_proxy.o : tcp_proxy.c
# 	gcc -c tcp_proxy.c 
# tcp_sock.o : tcp_sock.c
# 	gcc -c tcp_sock.c 
# tcp_conf.o : tcp_conf.c
# 	gcc -c tcp_conf.c
.PHONY : clean
clean :
	rm -rf $(obj) $(target)