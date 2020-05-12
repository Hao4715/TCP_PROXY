#include "../header/tcp_conf.h"

#define proxy_nums 10
#define proxy_nums_add 5

struct pattern
{
    char *str;
    int str_len;
};


int is_ip_legal(char *ip,int len)
{
    int i=0;
    int sum=0;
    if(ip[0] == '.' || ip[len-2] == '.')
    {
        return 0;
    }
    for(i=0;i<len;i++)
    {
        if(ip[i] == '.' || ip[i] == '\n')
        {
            if(sum <0 || sum > 255)
                return 0;
            sum = 0;
        }
        else
        {
            sum = sum*10 + (ip[i] - '0');
        }
    }
    return 1;
}

void block_read(FILE *file, int *block_num,int *ch,struct proxy *proxy_info)
{
    int i=0,len,i_ip;
    struct pattern pat[18] = {{"proxy",5},{"{",1},{"listen",6},{"{",1},{"port",4},{"",0},{";",1},{"}",1},{"server",6},{"{",1},{"ip",2},{"",0},{";",1},{"port",4},{"",0},{";",1},{"}",1},{"}",1}};
    while(i < 18)
    {
        switch (*ch)
        {
            case '#':
                while((*ch = fgetc(file)) != '\n');
                *ch = fgetc(file);
                break;
            case ' ':
            case '\n':
                *ch = fgetc(file);
                break;
            default:
                switch (i)
                {
                    case 5:
                        while((*ch - '0') >=0 && (*ch - '0') <= 9)
                        {
                            proxy_info[*block_num].listen_port  = proxy_info[*block_num].listen_port * 10 + (*ch -'0');
                            if(proxy_info[*block_num].listen_port > 65535)
                            {
                                printf("conf error in block:%d listen_port is to large!!\n",*block_num + 1);
                                exit(1);
                            }
                            *ch = fgetc(file);
                        }
                        //printf("listen_port : %d\n",proxyInfo[*blockNum].listen_port);
                        i++;
                        break;
                    case 11:
                        i_ip = 0;
                        while(( (*ch -'0') >= 0 && (*ch - '0') <= 9) || *ch == '.')
                        {
                            proxy_info[*block_num].server_ip[i_ip] = *ch;
                            *ch = fgetc(file);
                            i_ip++;
                        }
                        proxy_info[*block_num].server_ip[i_ip] = '\n';
                        if( is_ip_legal(proxy_info[*block_num].server_ip,i_ip+1) == 0)
                        {
                            printf("conf error in block:%d server_ip is illegal!!\n",*block_num+1);
                            exit(1);
                        }
                        //printf("server_ip : %s",proxyInfo[*blockNum].server_ip);
                        i++;
                        break;
                    case 14:
                        while ((*ch - '0') >= 0 && (*ch - '0') <= 9)
                        {
                            proxy_info[*block_num].server_port = proxy_info[*block_num].server_port * 10 + (*ch - '0');
                            if (proxy_info[*block_num].server_port > 65535)
                            {
                                printf("conf error in block:%d server_port is to large!!\n", *block_num +1);
                                exit(1);
                            }
                            *ch = fgetc(file);
                        }
                        //printf("server_port : %d\n", proxyInfo[*blockNum].server_port);
                        i++;
                        break;
                    default:
                        len = 0;
                        while (len < pat[i].str_len)
                        {
                            //printf("%c", *ch);
                            if (*ch != pat[i].str[len])
                            {
                                if( i == 6 || i == 12 || i == 15)
                                    printf("conf file error in block:%d around word:%s\n", *block_num+1, pat[i-2].str);
                                else 
                                    printf("conf file error in block:%d around word:%s\n", *block_num+1, pat[i].str);
                                exit(1);
                            }
                            len++;
                            *ch = fgetc(file);
                        }
                        i++;
                        //printf("\n");
                        break;
                    }
                break;
        }
    }
    *block_num += 1;
    return;
}

struct proxy * new_proxy(struct proxy* old_proxy_info,int *proxy_num,int block_num)
{
    *proxy_num += proxy_nums_add;
    struct proxy * new_proxy_info = (struct proxy*)malloc(sizeof(struct proxy) * (*proxy_num));
    memset(new_proxy_info,0,sizeof(struct proxy) * (*proxy_num));
    int i;
    for(i=0;i<block_num;i++)
    {
        new_proxy_info[i].listen_port = old_proxy_info[i].listen_port;
        new_proxy_info[i].server_port = new_proxy_info[i].server_port;
        strcpy(new_proxy_info[i].server_ip,old_proxy_info[i].server_ip);
    }
    return new_proxy_info;
}

struct proxy* get_conf_info(int *proxy_num)
{
    FILE *file = fopen("tcp_proxy.conf","r");
    int ch,i;
    int proxy_info_len = proxy_nums;
    struct proxy *proxy_info = (struct proxy *)malloc(sizeof(struct proxy)*proxy_nums);
    memset(proxy_info,0,sizeof(struct proxy)*proxy_nums);
    while((ch = fgetc(file)) != EOF)
    {
        if(ch == '#')
        {
            while((ch = fgetc(file)) != '\n');
        }
        else if(ch == ' ' || ch == '\n')
        {
            continue;
        }
        else
        {
            if(*proxy_num == proxy_info_len)
            {
                struct proxy *delete = proxy_info;
                proxy_info = new_proxy(proxy_info,&proxy_info_len,*proxy_num);
                free(delete);
                delete = NULL;
            }
            block_read(file,proxy_num,&ch,proxy_info);
        }
    }
    // for(i=0;i<*proxyNum;i++)
    // {
    //     printf("proxy %d : \n     port:%d ;  ip:%s ; port:%d;\n",i,proxyInfo[i].listen_port,proxyInfo[i].server_ip,proxyInfo[i].server_port);
    // }
    fclose(file);
    return proxy_info;
}
