#ifndef _WINSOCK_H_ 
#define _WINSOCK_H_ 

#include <winsock2.h>
#include <ws2tcpip.h>
#include "macro_define.h"

extern SOCKET	g_hSocket;
extern HANDLE 	g_hCompletionPort;
extern HANDLE	g_hReadEvent;
extern UCHAR	achInBuf [BUFSIZE];

void InitWinsock2();
void UnInitWinsock2();
#endif
