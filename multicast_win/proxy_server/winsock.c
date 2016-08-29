#include <stdio.h>
#include "winsock.h"
#include "config.h"
#include "handleIncomeData.h"

SOCKET	g_hSocket;
HANDLE 	g_hCompletionPort;
HANDLE	g_hReadEvent;
UCHAR	achInBuf [BUFSIZE];

/*
 * func:    win socket init 
 * param:   
 * return:  
 */
void InitWinsock2()
{
	WSADATA data;
	WORD version;
	int ret = 0;	

	version = (MAKEWORD(2, 2));
	ret = WSAStartup(version, &data);
	if (ret != 0)
	{
		ret = WSAGetLastError();
		if (ret == WSANOTINITIALISED)
		{
			fprintf(gfp_log, "[%s:%d]not initialised\n", __FILE__, __LINE__);
		}
   }
}

/*
 * func:    win socket uninit 
 * param:   
 * return:  
 */
void UnInitWinsock2()
{
	WSACleanup();
}
