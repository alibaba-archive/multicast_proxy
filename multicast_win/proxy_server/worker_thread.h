#ifndef _WORKER_THREAD_H_
#define _WORKER_THREAD_H_

#include "winsock.h"
#include "config.h"

DWORD WINAPI handle_work(void *param);
DWORD WINAPI worker_thread(void *dev); 
#endif 
