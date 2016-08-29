#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include "connectInit.h"
#include "config.h"
#include "log.h"

SOCKET   g_hSocket;
unsigned char achInBuf [BUFSIZE];

/*
 * func:    create socket and bind it to IOCP 
 * param:   groupIp ip list of multi ip; groupNum the count of ip 
 * return:  0 success; FALSE 1. 
 */

bool  CreateNetConnections (unsigned int groupIp[], int groupNum)
{
    bool fFlag = true;
    int nRet = 0;
    struct sockaddr_in stLclAddr;
    struct ip_mreq stMreq;    // Multicast interface structure 
    // Get a datagram socket
    g_hSocket = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    //g_hSocket = socket(AF_INET, SOCK_RAW, IPPROTO_UDP|IPPROTO_TCP|IPPROTO_ICMP);
    if (g_hSocket == -1) 
    {
        fprintf(gfp_log, "[%s:%d]socket() failed, Err: %d\n", __FILE__, __LINE__, errno);
        return false;
    }
    nRet = setsockopt(g_hSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&fFlag, sizeof(fFlag));
    if (nRet != 0) 
    {
        fprintf(gfp_log, "[%s:%d]setsockopt() SO_REUSEADDR failed, Err: %d\n", __FILE__, __LINE__, errno);
    }
    bool bOpt = true;
    nRet = setsockopt(g_hSocket, SOL_SOCKET, SO_BROADCAST, (char *)&bOpt, sizeof(bOpt));
    if (nRet != 0) 
    {
        fprintf(gfp_log, "[%s:%d]setsockopt() SO_REUSEADDR failed, Err: %d\n", __FILE__, __LINE__, errno);
    }

    nRet = setsockopt(g_hSocket, IPPROTO_IP, IP_HDRINCL, (char *)&fFlag, sizeof(fFlag));
    if (nRet != 0) 
    {
        fprintf(gfp_log, "[%s:%d]setsockopt() IP_HDRINCL failed, Err: %d\n", __FILE__, __LINE__, errno);
    }

    // Name the socket (assign the local port number to receive on) 
    stLclAddr.sin_family      = AF_INET;
    stLclAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    stLclAddr.sin_port        = htons(0);
    nRet = bind(g_hSocket,(struct sockaddr*) &stLclAddr,sizeof(stLclAddr));
    if (nRet == -1) 
    {
        fprintf(gfp_log, "[%s:%d]bind() failed, Err: %d\n", __FILE__, __LINE__, errno);
    }
    //here gmax_multi_ip gmin_multi_ip should get from config.c , change later!!!!!!!:
    // Join the multicast group so we can receive from it 
    int i;
    for(i=0; i<groupNum; i++)
    {
        if(ntohl(groupIp[i]) > gmax_multi_ip ||
                ntohl(groupIp[i]) < gmin_multi_ip)
        {
            continue;
        }
        stMreq.imr_multiaddr.s_addr = groupIp[i];
        stMreq.imr_interface.s_addr = htonl(INADDR_ANY);
        nRet = setsockopt(g_hSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&stMreq, sizeof(stMreq));
        if (nRet != 0) 
        {
            fprintf(gfp_log, "[%s:%d]setsockopt() IP_ADD_MEMBERSHIP address %s failed, Err: %d\n", __FILE__, __LINE__, inet_ntoa(*(struct in_addr *)&groupIp[i]), errno);
            fflush(gfp_log);
        }
    }
    return true;
}
