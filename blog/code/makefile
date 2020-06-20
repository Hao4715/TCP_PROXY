src = $(wildcard ./src/*.c)
obj = $(patsubst %.c, %.o, $(src))

target = blog
CC = gcc

$(target): $(obj)
	$(CC) $(obj) -o $(target) -g
%.o: %.c
	$(CC) -c $< -o $@

.PHONY : clean
clean :
	rm -rf $(obj) $(target)