#ifndef connectInit_h
#define connectInit_h
#include "macro_define.h"
#define true 1
#define false 0
typedef int bool;
typedef int SOCKET;


extern SOCKET	g_hSocket;
extern unsigned char achInBuf [BUFSIZE];

bool CreateNetConnections (unsigned int groupIp[], int groupNum);
#endif
