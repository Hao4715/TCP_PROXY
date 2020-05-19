#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <net/ethernet.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>

void print_ether_header_info(struct ether_header *eptr){
    int i;
    u_char *ptr;

    printf("Ethernet II:\n");
    ptr = eptr->ether_dhost; //目的MAC
    i = ETHER_ADDR_LEN;//6个八字节，ether头部中地址长度为6
    printf("Destination MAC address: ");
    do
    {
        printf("%s%02x", (i == ETHER_ADDR_LEN)? "":":",*ptr++);//02表示两位长度不够补零，x十六进制整数
    } while (--i>0);
    printf("\n");

    ptr = eptr->ether_shost; //源MAC
    i = ETHER_ADDR_LEN;
    printf("Source MAC address: ");
    do
    {
        printf("%s%02x", (i == ETHER_ADDR_LEN)? "":":",*ptr++);
    } while (--i>0);
    printf("\n");
    
    printf("Ethernet type: %#4x ", ntohs(eptr->ether_type)); //负载类型
    if(ntohs(eptr->ether_type) == ETHERTYPE_IP)
        printf("(IPv4)\n");
    else if (ntohs(eptr->ether_type) == ETHERTYPE_ARP)
        printf("(ARP)\n");
    else if (ntohs(eptr->ether_type) == ETHERTYPE_REVARP)
        printf("(RARP)\n");
    else
        printf("(Other)\n");     
}

void print_ip_header_info(struct iphdr *ipptr)
{
    struct in_addr addr;
    char *ch;
    ch = (char *)(ipptr);
    printf("\nInternet Protocol:\n");
    printf("Version: %d\n",ipptr->version);
    printf("Header length: %d\n",ipptr->ihl);
    printf("Total length: %d\n",ntohs(ipptr->tot_len));
    printf("Identifiction: %#04x\n",ipptr->id);

    ch += 6;

    printf("Flags: 0x%02x%02x",(*ch),(*(ch + 1)));

    if((*ch)==0x40 && (*(ch + 1)) == 0x00)
        printf("Don't fragment\n");
    else printf("More fragment\n");
    printf("Time to live: %d\n",ipptr->ttl);
    printf("Protocol: ");
    if(ipptr->protocol == 1)
        printf("ICMP(1)\n");
    else if (ipptr->protocol == 2)
        printf("IGMP(2)\n");
    else if (ipptr->protocol == 6)
        printf("TCP(6)\n");
    else printf("Unknow(%d)\n",ipptr->protocol);
    printf("Header checksum : %#04x\n",ipptr->check);
    addr.s_addr = ipptr->saddr;
    printf("Source IP address: %s\n",inet_ntoa(addr));
    addr.s_addr =  ipptr->daddr;
    printf("Destination IP address: %s\n",inet_ntoa(addr));
}

//输出TCP头部信息
void print_tcp_header_info(struct tcphdr *tcpptr){
	printf("\nTransmission Control Protocol(TCP):\n"); 
	printf("Source port: %d\n",ntohs(tcpptr->source));//源端口
	printf("Destination port: %d\n",ntohs(tcpptr->dest));//目的端口
	printf("Seq: %u\n",ntohs(tcpptr->seq));//确认号
	printf("Checksum: %#04x\n",tcpptr->check);//检验和
	printf("state: %u\n",ntohs(tcpptr->seq));//检验和
}

//输出UDP头部信息
void print_udp_header_info(struct udphdr *udpptr){
	printf("\nUser Datagram Protocol(UDP):\n");
	printf("Source port: %d\n",ntohs(udpptr->source));//源端口
	printf("Destination port: %d\n",ntohs(udpptr->dest));//目的端口
	printf("Length: %d\n",ntohs(udpptr->len));//长度
	printf("Checksum: %#04x\n",udpptr->check);//检验和
}

//输出数据信息
void print_data_info(int j,const struct pcap_pkthdr *pkthdr,const u_char *packet){
	int i;
	printf("\nData:\n");
	for(i=0;j<pkthdr->len;i++,j++){
		printf(" %02x",packet[j]);
		if((i+1)%16==0)
			printf("\n");
	}
	printf("\n");
}


void printf_info(u_char *arg, const struct pcap_pkthdr *pkthdr, const u_char *packet)
{
    int *id = (int *)arg,j =0;
    struct ether_header *eptr;
    struct iphdr *ipptr;
    struct tcphdr *tcpptr;
    struct udphdr *udpptr;

    printf("Packet Count : %d \n", ++(*id));
    printf("Recvied Packet Size:%d\n",pkthdr->len);
    printf("Number of bytes: %d\n",pkthdr->caplen);
    printf("Received time: %s\n",ctime((const time_t*)&pkthdr->ts.tv_sec));
    printf("Payload:\n");

    eptr = (struct ether_header*)packet;
    print_ether_header_info(eptr);
    j += 14;
    
    if(ntohs(eptr->ether_type) == ETHERTYPE_IP)
    {
        ipptr = (struct iphdr *)(packet + sizeof(struct ether_header));
        print_ip_header_info(ipptr);

        j += ipptr->ihl;

        if(ipptr->protocol == 6)
        {
            //获取tcp数据包
            tcpptr = (struct tcphdr *)(packet + sizeof(struct ether_header) + sizeof(struct iphdr));
            print_tcp_header_info(tcpptr);
            j += 20;
        } 
        else if(ipptr->protocol == 17)
        {
            //获取udp数据包
            udpptr = (struct udphdr *)(packet + sizeof(struct ether_header) + sizeof(struct iphdr));
            print_udp_header_info(udpptr);
            j += 20;
        }
        else printf("\nThis packet didn't use TCP/UDP protocol,%d.\n",ipptr->protocol);
    }
    else printf("\nThis packet didn't use IP protocol.\n");

    print_data_info(j,pkthdr,packet);
    printf("\n\n");
}


int main()
{
    char errBuf[PCAP_ERRBUF_SIZE];
    char *device = "ens33";
    pcap_t *head;
    struct bpf_program fp;
    int id,res;

    head = pcap_open_live(device,65535,0,1000,errBuf);

    //res = pcap_compile(head,&fp,"src or dst host 192.168.1.4",1,0);
    res = pcap_compile(head,&fp,"tcp port 8999",1,0);
    if(res != 0)
    {
        printf("pcap_compile error\n");
        exit(1);
    }
    res = pcap_setfilter(head,&fp);
    if(res != 0)
    {
        printf("pcap_setfilter error\n");
        exit(1);
    }

    if(head)
    {
        printf("open success\n");
        id = 0;
        pcap_loop(head,-1,printf_info,(u_char *)&id);
    }
    else 
    {
        printf("open error. %s\n",errBuf);
        printf("Get a packet failed. %s\n",errBuf);
        pcap_close(head);
        return 0;
    }

    //struct pcap_pkthdr packet;
    //const __u_char *packetflag = pcap_next(head,&packet);

    /*if(packetflag)
    {
        printf("Get a packet success.\n");
        // printf("Packet length:%d\n",packet.len);
        // printf("Number of byts: %d\n",packet.caplen);
        // printf("Received time: %s\n",ctime((const time_t*)&packet.ts.tv_sec));
        id = 0;
        pcap_loop(head,1,printf_info,(u_char *)&id);
    }
    else
    {
        printf("Get a packet failed. %s\n",errBuf);
        pcap_close(head);
        return 0;
    }*/

    pcap_close(head);

    return 0;
}