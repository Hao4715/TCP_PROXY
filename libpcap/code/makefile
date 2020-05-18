src = $(wildcard ./*.c)
obj = $(patsubst %.c, %.o, $(src))
target = pcap
CC = gcc

$(target): $(obj)
	$(CC) $(obj) -o $(target) -lpcap
%.o: %.c
	$(CC) -c $< -o $@

.PHONY : clean
clean :
	rm -rf $(obj)
