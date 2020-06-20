#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
// create listen cockfd
int create_listenfd()
{
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	int opt = 1;

	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in server_addr;
	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(80);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	int res = bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if(res == -1)
	{
		perror("bind error\n");
	}
	listen(server_fd,100);
	return server_fd;
}

void handle_request(int client_fd)
{
	char buffer[1024*1024] = {0};
	int request_info = read(client_fd, buffer, sizeof(buffer));
	printf("request_info: %s\n",buffer);

	char filename[50] = {0};
	sscanf(buffer, "GET /%s",filename);
	printf("filename: %s\n",filename);
	
	char *mime = NULL;
	if(strstr(filename,".html"))
	{
		mime = "text/html";
	}
	else if(strstr(filename, ".jpg") || strstr(filename, ".ico"))
	{
		mime = "image/jpg";
	}
	else if(strstr(filename,".css"))
	{
		mime = "text/css";
	}
	else if(strstr(filename,".js"))
	{
		mime = "text/javascript";
	}else if (strstr(filename,".mp3"))
	{
		mime = "audio/mpeg";
	}
	else if (strstr(filename,".gif"))
	{
		mime = "image/gif";
	}
	else
	{
		perror("mime type error\n");
	}


	char response[1024*1024] = {0};
	sprintf(response,"HTTP/1.1 200 OK\r\nContent-Type: %s; charset=UTF-8\r\n\r\n",mime);
	int head_len = strlen(response);

	int file_fd = open(filename,O_RDONLY);
	int file_len = read(file_fd,response + head_len,sizeof(response)-head_len);

	write(client_fd,response,head_len + file_len);
	close(file_fd);

}

int main(void)
{
	int server_fd = create_listenfd();
	printf("sockfd : %d\n", server_fd);
	
	
	while(1)
	{
		int client_fd = accept(server_fd, NULL, NULL);
		printf("accept success\n");
		handle_request(client_fd);
		close(client_fd);
	}
	close(server_fd);
	return 0;
}

