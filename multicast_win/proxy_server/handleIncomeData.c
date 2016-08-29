#include <stdio.h>
#include <winsock2.h>
#include <errno.h>
#include "winsock.h"
#include "iphdr.h"
#include "config.h"
#include "multi_grp.h"

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
int HandleIncomingData( UCHAR* pBuf, int bufLen, int type)
{
    grecv_pkt++; 
#if 0	
    char buf[BUFSIZE];
    memcpy(buf, pBuf, bufLen);
    //memset(pBuf, 0, BUFSIZE);
    pBuf = (UCHAR *)buf;
#endif
    struct ip_hdr *iph = (struct ip_hdr *)(pBuf);
    int iph_len = 4 * (iph->ip_verlen & 0x0F);
    struct udp_hdr *udph = (struct udp_hdr *)(pBuf + iph_len);

    uint32_t multi_ip = iph->ip_destaddr;
    uint16_t multi_port = ntohs(udph->dest_portno);
    uint32_t ip_list[MAX_MEM_IP_IN_GROUP];
    struct multi_node node;
    /*
     * according to the multi_ip to send to point
     */
    int rt = lookup_multi_node(multi_ip, multi_port, ip_list, &node);
    if(rt != 0)
    {
        //printf("lookup_multi_node is null!\n");
        gdrop_pkt++; 
        return -1;
    } 
#if 0	
    char srcip[16];
    strcpy(srcip, inet_ntoa(*((struct in_addr *)&iph->ip_srcaddr)));
    fprintf(stderr, "sip->dip[%s %s]\n\n", srcip, inet_ntoa(*((struct in_addr *)&iph->ip_destaddr)));
#endif 
#if 0	
    /* just for test */
    if(ntohs(udph->dest_portno) != 7127)
    {
        return -1;
    } 
#endif
    gforward_pkt++;
    int i_ip;
    int send_len = ntohs(iph->ip_totallength);
    for(i_ip=0; i_ip<node.multi_member_cnt; i_ip++)
    { 
        /* here according to the ttl and the ip_id to judge the pkt is not the proxy sending */
        if(0x80 == iph->ip_ttl && 0x1000 == htons(iph->ip_id))
        {
            printf("this is the local multicast!\n"); 
            return -1;
        }

        SOCKET sendSocket = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
        if(sendSocket == INVALID_SOCKET)
        {
            fprintf(gfp_log, "[%s:%d]send socket error %d\n", __FILE__, __LINE__, WSAGetLastError());
            fflush(gfp_log);
            return -1;
        }

        //here dealing with the data and restruct the packet
        iph->ip_destaddr = ip_list[i_ip];//inet_addr("10.1.36.255");
        iph->ip_checksum = 0;
        iph->ip_ttl = 127;
        udph->udp_checksum = 0;
        udph->dest_portno = htons(node.server_port);

        iph->ip_checksum = (checksum((unsigned short *)iph, iph_len));
        udph->udp_checksum = (udp_checksum(iph, iph_len, ntohs(udph->udp_length)));



        BOOL fFlag = TRUE;
        int nRet = setsockopt(sendSocket, IPPROTO_IP, IP_HDRINCL, (char *)&fFlag, sizeof(fFlag));
        if (nRet == SOCKET_ERROR) 
        {
            fprintf(gfp_log, "[%s:%d]setsockopt() IP_HDRINCL failed, Err: %d\n", __FILE__, __LINE__, WSAGetLastError());
            return -1;
        }

        /* Assign our destination address */
        SOCKADDR_IN stDstAddr;
        stDstAddr.sin_family =      AF_INET;
        stDstAddr.sin_addr.s_addr = ip_list[i_ip];
        stDstAddr.sin_port =        htons(node.server_port);

        nRet = sendto(sendSocket, 
                (char *)pBuf, 
                send_len,
                0,
                (struct sockaddr*)&stDstAddr, 
                sizeof(stDstAddr));	
        if(nRet < 0)
        {
            fprintf (gfp_log, "[%s:%d]sendto() failed, Error: %d\n", __FILE__, __LINE__, WSAGetLastError());

            return -1;
        }

        //fprintf(stderr, "====send yes! udplen[%d] iptotallen[%d] bufLen[%d] type=[%d]\n", 
        //	ntohs(udph->udp_length), ntohs(iph->ip_totallength), bufLen, type);

        gforward_point_pkt++; 
        closesocket(sendSocket);
    }
    return 0;
}


