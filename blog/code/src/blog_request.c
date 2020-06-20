#include "../header/blog_request.h"


int get_file_fd(char * buffer)
{
    char * url = (char *)malloc(sizeof(char) * 256);
    sscanf(buffer,"GET /%s HTTP/",url);
    printf("url: %s\n",url);
    int file_fd = open(url,O_RDONLY, 0);
    if(file_fd == -1)
    {
        printf("file open error\n");
        return -1;
    }
    return file_fd;
}