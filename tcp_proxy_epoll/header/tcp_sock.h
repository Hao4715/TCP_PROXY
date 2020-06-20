#ifndef _TCP_SOCK_H
#define _TCP_SOCK_H
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <pthread.h>
#include <time.h>


int create_listenfd(int listen_port);

#endif