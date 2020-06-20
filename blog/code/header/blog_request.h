#ifndef _BLOG_REQUEST_H
#define _BLOG_REQUEST_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>

int get_file_fd(char * buffer);

#endif