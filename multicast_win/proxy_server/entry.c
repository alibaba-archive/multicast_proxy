#include <stdio.h> 
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <time.h>
#include "iphdr.h"
#include "config.h"
#include "multi_grp.h"
#include "worker_thread.h"
#include "handleIncomeData.h"
#include "queue.h"

unsigned int groupIp[MAX_GROUP];
int groupNum;

int gqueue_num = QUEUE_COUNT;

pcap_if_t *alldevs;
char errbuf[PCAP_ERRBUF_SIZE];
char gbpf_filter_string[FILTER_STRING_SIZE]; //过滤规则字符串，只分析IPv4的数据包
long finish_flag = 0; 
int count_dev = 0;

int get_dev_name_init()
{
    /* Retrieve the device list */
    if(pcap_findalldevs(&alldevs, errbuf) == -1)
    {
        fprintf(gfp_log, "[%s:%d]Error in pcap_findalldevs: %s\n", __FILE__, __LINE__, errbuf);
        fflush(gfp_log);
        return -1;
    }
    return 0;
}

int entry()
{
    printf("*******************************************************\n");
    printf("*This is the program changing pkt from multi to point!*\n");
    printf("*******************************************************\n");

    //Initialize
    log_init(SERVER_LOG_PATH);
    multi_ip_max_min_init();
    multi_node_init();

    srand((unsigned)time(NULL));
    int iq;
    for(iq=0; iq<gqueue_num; iq++)
    {
        init_queue(&pkt_queue[iq]);
    }

    int rt = cfg_init(SERVER_CFG, groupIp, &groupNum);
    if(rt != 0)
    {
        fprintf(gfp_log, "[%s:%d]cfg_init error!\n", __FILE__, __LINE__);
        fflush(gfp_log);
    }

    InitWinsock2();

    //create thread to reload cfg
    DWORD ThreadId;
    HANDLE ThreadHandle;
    ThreadHandle = CreateThread (NULL, 0, terminal_command, NULL, 0, &ThreadId);
    if (!ThreadHandle)
    {
        fprintf (gfp_log, "[%s:%d]Create cfgReload Thread Failed\n", __FILE__, __LINE__);
        fflush(gfp_log);
        return -1;
    }
    CloseHandle (ThreadHandle);

    //We want to keep the main thread running
    HANDLE hWait2Exit = CreateEvent(NULL, FALSE, TRUE, "MSERVER");
    ResetEvent(hWait2Exit);

    //This OVERLAPPED event
    g_hReadEvent = CreateEvent(NULL, TRUE, TRUE, NULL); 
    if(g_hReadEvent == NULL)
    {
        fprintf(gfp_log, "[%s:%d]CreateEvent error [%u]\n", __FILE__, __LINE__, GetLastError());
    }

    //
    // try to get timing more accurate... Avoid context
    // switch that could occur when threads are released
    //

    SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    get_dev_name_init();

    int i = 0;
    pcap_if_t *d;
    HANDLE ThreadHandleWork[16];
    DWORD ThreadIdWork;

    for(d=alldevs; d; d=d->next)
    {
        count_dev++;
    }

    //fprintf(stderr, "count: %d\n", count_dev);
    for(d=alldevs; d; d=d->next)
    {
        ThreadHandleWork[i] = CreateThread (NULL, 0, worker_thread, d, 0, &ThreadIdWork);
        if (!ThreadHandleWork[i])
        {
            fprintf (gfp_log, "[%s:%d]Create work Thread Failed\n", __FILE__, __LINE__);
            return -1;
        }

        i++;
    }

    int id;
    HANDLE ThreadHandleData[QUEUE_COUNT];
    SYSTEM_INFO sys_info;

    GetSystemInfo(&sys_info);
    gqueue_num = sys_info.dwNumberOfProcessors;
    if(gqueue_num > QUEUE_COUNT)
    {
        gqueue_num = QUEUE_COUNT;
    }

    for(id=0; id<gqueue_num; id++)
    {
        int *queue_id = (int *)malloc(sizeof(int));
        *queue_id = id; 
        ThreadHandleData[id] = CreateThread (NULL, 0, handle_work, queue_id, 0, &ThreadIdWork);
        if (!ThreadHandleData[id])
        {
            fprintf (gfp_log, "[%s:%d]Create handle data Thread Failed\n", __FILE__, __LINE__);
            return -1;
        }
    }


    WaitForSingleObject(hWait2Exit, INFINITE);
    int j;
    for(j=0; j<i; j++)
    {
        WaitForSingleObject(ThreadHandleWork[j], INFINITE);
        CloseHandle (ThreadHandleWork[j]);
    }
    for(j=0; j<id; j++)
    {
        WaitForSingleObject(ThreadHandleData[j], INFINITE);
        CloseHandle (ThreadHandleData[j]);
    }

    UnInitWinsock2();
    log_uninit(); 
    return 1;
}
