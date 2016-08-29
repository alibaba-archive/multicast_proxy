#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <linux/if_packet.h>
#include <linux/if_tun.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/sockios.h>
#include <errno.h>
#include <pcap.h>
#include "dev_pcap.h"
#include "handleData.h"
#define UDP_HEADER_SIZE 8
#define IPPROTO_VRRF 112
#define PCAP_OPENFLAG_PROMISCUOUS 1
unsigned int grecv_pkt;
unsigned int gdrop_pkt;
unsigned int gforward_pkt;
extern int tun;
/*
 * func: 	dealing with the data coming.
 * param: 	pBuf points to the coming data.
 *			bufLen is the length of data in buf.
 * return: 	TRUE success; FALSE fail
 */
int HandleIncomingData(unsigned char * pBuf, int bufLen)
{
    grecv_pkt++;
    char buf[BUFSIZE];
    if(bufLen < BUFSIZE - ETHER_HEADER_SIZE) 
    {
        memcpy(buf + ETHER_HEADER_SIZE, pBuf, bufLen);
    }
    struct eth_hdr *pethh = (struct eth_hdr *)buf;
    struct ip_hdr *piph = (struct ip_hdr*)(buf + ETHER_HEADER_SIZE);

    int iph_len = 4 * (piph->ip_verlen & 0x0F);
    struct udp_hdr * pudph;
    if( piph->ip_protocol == IPPROTO_UDP )
	  pudph = (struct udp_hdr*)(buf + ETHER_HEADER_SIZE + iph_len);
    else 
          pudph = NULL;

    uint32_t server_ip = piph->ip_srcaddr;
    uint16_t server_port;
    if( piph->ip_protocol == IPPROTO_UDP )
          server_port = ntohs(pudph->dest_portno);
    else if ( piph->ip_protocol == IPPROTO_VRRF )
          server_port = 7126;
    else  server_port = 0;
    uint32_t multi_ip;
    uint16_t multi_port;
    uint32_t index, row_id;
    //printf("there is a pkt from %s!\n", inet_ntoa(*(struct in_addr *)&server_ip));
    /* dest ip judgement, if it is not the local host, dropped. 
     */ 

    /* search the hash table, judge the source ip
     */
    int rt;
    rt = lookup_ip_port_node(server_ip, 
            server_port, 
            &multi_ip, 
            &multi_port, 
            &index, 
            &row_id);
    if(rt == -1)
    {
        //printf("sip is not in the group!\n");
        gdrop_pkt++;
        return -1;
    }
    /* restruct the pkt
     */
    //printf("incoming package : %s \n",buf);
//    printf("incoming package:   len: %d",bufLen);
 /*   int it=0;
    for (it = 0;it!=bufLen;it++)
    {
       printf("%c",buf[it]);
    }*/
    /////////////////////////////////////////
    if(piph->ip_protocol == IPPROTO_UDP)
    { 
        restruct_pkt(buf, pethh, piph, iph_len, pudph, multi_ip, multi_port);
    }
    else if(piph->ip_protocol == IPPROTO_VRRF)
    {
        restruct_pkt_others_proto(buf, pethh, piph, iph_len, pudph, multi_ip, multi_port);
    }
    gforward_pkt++;

    return true;
}

/*
 * func: 	restruct a new udp package.
 * param: 	
 * return: 	0 success; -1 fail
 */
int restruct_pkt(char *buf, 
        struct eth_hdr *pethh, 
        struct ip_hdr *piph, int iph_len, 
        struct udp_hdr *pudph,
        uint32_t multi_ip,
        uint16_t multi_port)
{
    char errbuf[PCAP_ERRBUF_SIZE] = {0};
   // char * device = "eth0";
    char device [DEV_BUF_SIZE];
    pcap_t *adhandle = NULL;
    int ret;

    char *str_dev = get_dev_name(piph->ip_destaddr, device);
    if(str_dev == NULL)
    {
        fprintf(gfp_log, "[%s:%d]get_dev_name is null!\n", __FILE__, __LINE__);
        fflush(gfp_log);
        return -1;
    }
    if((adhandle = pcap_open_live(device, 0x10000, PCAP_OPENFLAG_PROMISCUOUS, 1000,  errbuf)) == NULL)
    {
        fprintf(gfp_log, "[%s:%d][pcap_open_live error] : %s\n", __FILE__, __LINE__, errbuf);
        fflush(gfp_log);
        return -1;
    }

    pethh->ether_dhost[0] = 0x01;
    pethh->ether_dhost[1] = 0x00;
    pethh->ether_dhost[2] = 0x5e;
    pethh->ether_dhost[3] = (unsigned char)((multi_ip  >> 8) & 0x7F);;
    pethh->ether_dhost[4] = (unsigned char)((multi_ip >> 16) & 0xFF);;
    pethh->ether_dhost[5] = (unsigned char)((multi_ip >> 24) & 0xFF);;
//AC:85:3D:AF:C7:08
    pethh->ether_shost[0] = 0x00;
    pethh->ether_shost[1] = 0x16;
    pethh->ether_shost[2] = 0x3e;
    pethh->ether_shost[3] = 0x00;
    pethh->ether_shost[4] = 0x04;
    pethh->ether_shost[5] = 0x0e;

    pethh->ether_type = htons(ETHERTYPE_IP);

    if((sizeof(struct ip_hdr) % 4) != 0)
    {
        fprintf(gfp_log, "[%s:%d][IP Header error]\n", __FILE__, __LINE__);
        fflush(gfp_log);
        pcap_close(adhandle);
        return -1;
    }
    uint16_t ip_len = ntohs(piph->ip_totallength);
    uint16_t udp_len = htons(pudph->udp_length);

    piph->ip_tos = 0;
    piph->ip_id = htons(0x0000);
    piph->ip_offset = htons(0x4000);
    piph->ip_ttl = 0x20;
    piph->ip_checksum = 0;
    piph->ip_destaddr = multi_ip;
    piph->ip_totallength = htons(ip_len); 
    piph->ip_checksum  = checksum((uint16_t *)piph, iph_len);

    pudph->dest_portno = htons(multi_port);
    pudph->src_portno = htons(52540);
    pudph->udp_checksum = 0;
    pudph->udp_length = htons(udp_len);
    pudph->udp_checksum = udp_checksum(piph, iph_len, udp_len);
    //pudph->udp_checksum = htons(0x16ed);
    //////just for test printf
    //printf("rebuild multicast package: \n");
    //printf("   src : %s" , inet_ntoa(*((struct in_addr *)(&piph->ip_srcaddr))));
    //printf("   dst : %s\n" , inet_ntoa(*((struct in_addr *)(&piph->ip_destaddr))));
    ///////////////////////////////////////////////////
    int pkt_len = ip_len + ETHER_HEADER_SIZE;

    /* just print the pkt info */
    /*
    int i=0;
    for(i = 0 ; i != sizeof(struct ip_hdr); i++)
    {
        printf("%02x ",(*((char *)piph+i))&0xff);
    }
    */
    //printf("pkt len %u\n", pkt_len);
    ret = write(tun, piph, ip_len);
    if(pcap_sendpacket(adhandle, (const u_char*)buf, pkt_len) == -1)
    {
        fprintf(gfp_log, "[%s:%d][pcap_sendpacket error]\n", __FILE__, __LINE__);
        fflush(gfp_log);
        pcap_close(adhandle);
        return -1;
    }
    else
    {
     //   printf("=====send a pkt!\n");
    }
    pcap_close(adhandle);
    return 0;
}

/*
 * func: 	restruct a new others proto package.
 * param: 	
 * return: 	0 success; -1 fail
 */
int restruct_pkt_others_proto(char *buf, 
        struct eth_hdr *pethh, 
        struct ip_hdr *piph, int iph_len, 
        struct udp_hdr *pudph,
        uint32_t multi_ip,
        uint16_t multi_port)
{
    char errbuf[PCAP_ERRBUF_SIZE] = {0};
    char * device = "eth0";
    pcap_t *adhandle = NULL;

    if((adhandle = pcap_open_live(device, 0x10000, PCAP_OPENFLAG_PROMISCUOUS, 1000, errbuf)) == NULL)
    {
        fprintf(gfp_log, "[%s:%d][pcap_open_live error] : %s\n", __FILE__, __LINE__, errbuf);
        fflush(gfp_log);
        return -1;
    }

    pethh->ether_dhost[0] = 0x01;
    pethh->ether_dhost[1] = 0x00;
    pethh->ether_dhost[2] = 0x5e;
    pethh->ether_dhost[3] = 0x7e;
    pethh->ether_dhost[4] = 0x01;
    pethh->ether_dhost[5] = 0x02;

    pethh->ether_shost[0] = 0x00;
    pethh->ether_shost[1] = 0x16;
    pethh->ether_shost[2] = 0x3e;
    pethh->ether_shost[3] = 0x00;
    pethh->ether_shost[4] = 0x04;
    pethh->ether_shost[5] = 0x0e;

    pethh->ether_type = htons(ETHERTYPE_IP);

    if((sizeof(struct ip_hdr) % 4) != 0)
    {
        fprintf(gfp_log, "[%s:%d][IP Header error]\n", __FILE__, __LINE__);
        pcap_close(adhandle);
        return -1;
    }

    uint16_t ip_len = ntohs(piph->ip_totallength);

    piph->ip_tos = 0;
    piph->ip_id = htons(0x1000);
    piph->ip_offset = htons(0x0040);
    piph->ip_ttl = 0x80;
    piph->ip_checksum = 0;
    piph->ip_destaddr = multi_ip;
    piph->ip_totallength = htons(ip_len); 
    piph->ip_checksum = checksum((u_int16_t*)piph, iph_len);


    int pkt_len = ip_len + ETHER_HEADER_SIZE;
    if(pcap_sendpacket(adhandle, (const u_char*)buf, pkt_len) == -1)
    {
        fprintf(gfp_log, "[%s:%d][pcap_sendpacket error]\n", __FILE__, __LINE__);
        fflush(gfp_log);
        pcap_close(adhandle);
        return -1;
    }
    pcap_close(adhandle);
    return 0;
}

