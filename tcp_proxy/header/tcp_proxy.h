#ifndef _TCP_PROXY_H
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


void proxy_process(int listenPort, char *serverIp, int serverPort);
void *handle_request(void *arg);
#endif