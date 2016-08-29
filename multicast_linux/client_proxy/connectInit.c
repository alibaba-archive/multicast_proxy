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
#include "log.h"
#include "connectInit.h"
#include "handleData.h"

SOCKET	g_hSocket;

/*
 * func:    create socket and bind it to IOCP 
 * param:   groupIp ip list of multi ip; groupNum the count of ip 
 * return:  TRUE success; FALSE fail. 
 */

bool CreateNetConnections(void)
{
    bool fFlag = true;
    int nRet=0;
    struct sockaddr_in stLclAddr; 

    // Get a datagram socket
    g_hSocket = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    if (g_hSocket == -1) 
    {
        fprintf(gfp_log, "[%s:%d]socket() failed, Err: %d\n", __FILE__, __LINE__, errno);
        fflush(gfp_log);
        return false;
    }

    nRet = setsockopt(g_hSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&fFlag, sizeof(fFlag));
    if (nRet != 0) 
    {
        fprintf(gfp_log, "[%s:%d]setsockopt() SO_REUSEADDR failed, Err: %d\n", __FILE__, __LINE__, errno);
        fflush(gfp_log);
    }

    nRet = setsockopt(g_hSocket, IPPROTO_IP, IP_HDRINCL, (char *)&fFlag, sizeof(fFlag));
    if (nRet != 0) 
    {
        fprintf(gfp_log, "[%s:%d]setsockopt() IP_HDRINCL failed, Err: %d\n", __FILE__, __LINE__, errno);
        fflush(gfp_log);
    }

    // Name the socket (assign the local port number to receive on) 
    stLclAddr.sin_family      = AF_INET;
    stLclAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    stLclAddr.sin_port        = htons(0);
    nRet = bind(g_hSocket, (struct sockaddr*)&stLclAddr, sizeof(stLclAddr));
    if (nRet == -1) 
    {
        fprintf(gfp_log, "[%s:%d]bind() failed, Err: %d\n", __FILE__, __LINE__,errno);
        fflush(gfp_log);
    }


    return true;
}
