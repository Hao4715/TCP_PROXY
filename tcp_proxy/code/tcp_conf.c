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

void block_read(FILE *file, int *blockNum,int *ch,struct proxy *proxyInfo)
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
                            proxyInfo[*blockNum].listen_port  = proxyInfo[*blockNum].listen_port * 10 + (*ch -'0');
                            if(proxyInfo[*blockNum].listen_port > 65535)
                            {
                                printf("conf error in block:%d listen_port is to large!!\n",*blockNum + 1);
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
                            proxyInfo[*blockNum].server_ip[i_ip] = *ch;
                            *ch = fgetc(file);
                            i_ip++;
                        }
                        proxyInfo[*blockNum].server_ip[i_ip] = '\n';
                        if( is_ip_legal(proxyInfo[*blockNum].server_ip,i_ip+1) == 0)
                        {
                            printf("conf error in block:%d server_ip is illegal!!\n",*blockNum+1);
                            exit(1);
                        }
                        //printf("server_ip : %s",proxyInfo[*blockNum].server_ip);
                        i++;
                        break;
                    case 14:
                        while ((*ch - '0') >= 0 && (*ch - '0') <= 9)
                        {
                            proxyInfo[*blockNum].server_port = proxyInfo[*blockNum].server_port * 10 + (*ch - '0');
                            if (proxyInfo[*blockNum].server_port > 65535)
                            {
                                printf("conf error in block:%d server_port is to large!!\n", *blockNum +1);
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
                                    printf("conf file error in block:%d around word:%s\n", *blockNum+1, pat[i-2].str);
                                else 
                                    printf("conf file error in block:%d around word:%s\n", *blockNum+1, pat[i].str);
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
    *blockNum += 1;
    return;
}

struct proxy * new_proxy(struct proxy* oldProxyInfo,int *proxyNum,int blockNum)
{
    *proxyNum += proxy_nums_add;
    struct proxy * newProxyInfo = (struct proxy*)malloc(sizeof(struct proxy) * (*proxyNum));
    memset(newProxyInfo,0,sizeof(struct proxy) * (*proxyNum));
    int i;
    for(i=0;i<blockNum;i++)
    {
        newProxyInfo[i].listen_port = oldProxyInfo[i].listen_port;
        newProxyInfo[i].server_port = newProxyInfo[i].server_port;
        strcpy(newProxyInfo[i].server_ip,oldProxyInfo[i].server_ip);
    }
    return newProxyInfo;
}

struct proxy* get_conf_info(int *proxyNum)
{
    FILE *file = fopen("tcp_proxy.conf","r");
    int ch,i;
    int proxyInfoLen = proxy_nums;
    struct proxy *proxyInfo = (struct proxy *)malloc(sizeof(struct proxy)*proxy_nums);
    memset(proxyInfo,0,sizeof(struct proxy)*proxy_nums);
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
            if(*proxyNum == proxyInfoLen)
            {
                struct proxy *delete = proxyInfo;
                proxyInfo = new_proxy(proxyInfo,&proxyInfoLen,*proxyNum);
                free(delete);
                delete = NULL;
            }
            block_read(file,proxyNum,&ch,proxyInfo);
        }
    }
    // for(i=0;i<*proxyNum;i++)
    // {
    //     printf("proxy %d : \n     port:%d ;  ip:%s ; port:%d;\n",i,proxyInfo[i].listen_port,proxyInfo[i].server_ip,proxyInfo[i].server_port);
    // }
    fclose(file);
    return proxyInfo;
}
