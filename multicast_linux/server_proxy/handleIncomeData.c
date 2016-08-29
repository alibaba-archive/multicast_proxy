#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <errno.h>
#include "iphdr.h"
#include "config.h"
#include "multi_grp.h"
#include "macro_define.h"
#include "handleIncomeData.h"
#define UDP_HEADER_SIZE 8
#define IPPROTO_VRRF 112
unsigned int grecv_pkt = 0;
unsigned int gdrop_pkt = 0;
unsigned int gforward_pkt = 0;
unsigned int gforward_point_pkt = 0;

/*
 * func: 	dealing with the data coming.
 * param: 	pBuf points to the coming data.
 *			bufLen is the length of data in buf.
 * return: 	0 success; -1 fail
 */
int HandleIncomingData(unsigned char * pBuf, int bufLen, int type)
{
    grecv_pkt++; 
    char buf[BUFSIZE];

    memcpy(buf, pBuf, bufLen);
    memset(pBuf, 0, bufLen);
    pBuf = (unsigned char *)buf;
    int it=0;
    /*for(it = 0;it!=bufLen;it++)
    {
        printf("%c",buf[it]);
    }*/
    struct ip_hdr *iph = (struct ip_hdr *)(pBuf);
    int iph_len = 4 * (iph->ip_verlen & 0x0F);

    uint32_t multi_ip = iph->ip_destaddr;
    uint32_t ip_list[MAX_MEM_IP_IN_GROUP];
    struct multi_node node;
//    printf("   src : %s" , inet_ntoa(*((struct in_addr *)(&iph->ip_srcaddr))));
//    printf("   dst : %s\n" , inet_ntoa(*((struct in_addr *)(&iph->ip_destaddr))));
    /*
     * according to the multi_ip to send to point
     */
    int rt = lookup_multi_node(multi_ip, ip_list, &node);
    if(rt != 0)
    {
        gdrop_pkt++; 
        return -1;
    } 
    gforward_pkt++;
    if(iph->ip_protocol == IPPROTO_UDP )
    {   
        struct udp_hdr *udph = (struct udp_hdr *)(pBuf + iph_len);
        if(udph->dest_portno == htons(65533))
        {
            printf("recv client muticast pkt\n");    
        }
        int i_ip;
        /* add the gport and protocol to the end of udp data */
        int udp_len = ntohs(udph->udp_length);
//        int new_udp_len = udp_len + 3;
        short send_len = ntohs(iph->ip_totallength);

        for(i_ip=0; i_ip<node.multi_member_cnt; i_ip++)
        { 
            /* here according to the ttl and the ip_id to judge the pkt is not the proxy sending */
            if(0x80 == iph->ip_ttl && 0x1000 == htons(iph->ip_id))
            {
                return -1;
            }
           
            gforward_point_pkt++; 
            //short *payload_port = (short *)((char *)udph + udp_len); 
           // *payload_port = udph->dest_portno;

            //unsigned char *payload_proto = (unsigned char *)udph + udp_len + 2; 
           // *payload_proto = iph->ip_protocol;

            //here dealing with the data and restruct the packet
            iph->ip_destaddr = ip_list[i_ip];//inet_addr("10.1.36.255");
            iph->ip_checksum = 0;
            iph->ip_ttl = 127;
            //iph->ip_protocol = node.protocol;
            iph->ip_totallength = htons(send_len);
            iph->ip_checksum = (checksum((unsigned short *)iph, iph_len));

            udph->udp_checksum = 0;
            udph->dest_portno = htons(node.server_port);
            udph->udp_length = htons(udp_len);
            udph->udp_checksum = (udp_checksum(iph, iph_len, udp_len));

            SOCKET sendSocket = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
            if(sendSocket == -1)
            {
                fprintf(gfp_log, "[%s:%d]send socket error\n", __FILE__, __LINE__);
                fflush(gfp_log);
                return -1;
            }

            bool fFlag = true;
            int nRet = setsockopt(sendSocket, IPPROTO_IP, IP_HDRINCL, (char *)&fFlag, sizeof(fFlag));
            if (nRet != 0) 
            {
                fprintf(gfp_log, "[%s:%d]setsockopt() IP_HDRINCL failed, Err: %d\n", __FILE__, __LINE__, errno);
                fflush(gfp_log);
                return -1;
            }

            /* Assign our destination address */
            struct sockaddr_in stDstAddr;
            stDstAddr.sin_family =      AF_INET;
            stDstAddr.sin_addr.s_addr = ip_list[i_ip];
            stDstAddr.sin_port =        htons(node.server_port);

            nRet = sendto(sendSocket, 
                    buf, 
                    send_len,
                    0,
                    (struct sockaddr*)&stDstAddr, 
                    sizeof(stDstAddr));	
            if(nRet < 0)
            {
                fprintf (gfp_log, "[%s:%d]sendto() failed, Error: %d\n", __FILE__, __LINE__, errno);
                fflush(gfp_log);
                return -1;
            }

           // printf("====send yes! udplen[%d] iptotallen[%d] bufLen[%d] type=[%d]\n", 
             //       ntohs(udph->udp_length), ntohs(iph->ip_totallength), bufLen, type);

            close(sendSocket);
        }
    }
    else if(iph->ip_protocol == IPPROTO_VRRF)
    {  
	    printf("IPPROTO_VRRF\n");
        int i_ip;
        /* add the gport and protocol to the end of udp data */
//        int new_udp_len = udp_len + 3;
        short send_len = ntohs(iph->ip_totallength);

        for(i_ip=0; i_ip<node.multi_member_cnt; i_ip++)
        { 
            /* here according to the ttl and the ip_id to judge the pkt is not the proxy sending */
            if(0x80 == iph->ip_ttl && 0x1000 == htons(iph->ip_id))
            {
              //  printf("this is the local multicast!\n"); 
                return -1;
            }

            //short *payload_port = (short *)((char *)udph + udp_len); 
           // *payload_port = udph->dest_portno;

            //unsigned char *payload_proto = (unsigned char *)udph + udp_len + 2; 
           // *payload_proto = iph->ip_protocol;

            //here dealing with the data and restruct the packet
            iph->ip_destaddr = ip_list[i_ip];//inet_addr("10.1.36.255");
            iph->ip_checksum = 0;
            iph->ip_ttl = 127;
            //iph->ip_protocol = node.protocol;
            iph->ip_totallength = htons(send_len);
            iph->ip_checksum = (checksum((unsigned short *)iph, iph_len));


            SOCKET sendSocket = socket(AF_INET, SOCK_RAW, IPPROTO_IP);
            if(sendSocket == -1)
            {
                fprintf(gfp_log, "[%s:%d]send socket error\n", __FILE__, __LINE__);
                fflush(gfp_log);
                return -1;
            }

            bool fFlag = true;
            int nRet = setsockopt(sendSocket, IPPROTO_IP, IP_HDRINCL, (char *)&fFlag, sizeof(fFlag));
            if (nRet != 0) 
            {
                fprintf(gfp_log, "[%s:%d]setsockopt() IP_HDRINCL failed, Err: %d\n", __FILE__, __LINE__, errno);
                fflush(gfp_log);
                return -1;
            }

            /* Assign our destination address */
            struct sockaddr_in stDstAddr;
            stDstAddr.sin_family =      AF_INET;
            stDstAddr.sin_addr.s_addr = ip_list[i_ip];
            stDstAddr.sin_port =        htons(node.server_port);

            nRet = sendto(sendSocket, 
                    buf, 
                    send_len,
                    0,
                    (struct sockaddr*)&stDstAddr, 
                    sizeof(stDstAddr));	
            if(nRet < 0)
            {
                fprintf (gfp_log, "[%s:%d]sendto() failed, Error: %d\n", __FILE__, __LINE__, errno);
                fflush(gfp_log);
                return -1;
            }

            //printf("====send yes!  iptotallen[%d] bufLen[%d] type=[%d]\n", 
              //       ntohs(iph->ip_totallength), bufLen, type);

            gforward_point_pkt++; 
            close(sendSocket);
    }
    return 0;
}
}
