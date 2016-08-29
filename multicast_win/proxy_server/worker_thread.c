#include <stdio.h> 
#include <winsock2.h>
#include "worker_thread.h"
#include "handleIncomeData.h"
#include "queue.h"
#include "log.h"

#define PCAP_NETMASK_UNKNOWN	0xffffffff

extern int gqueue_num;
/*
 * func:     
 * param:   
 * return:  
 */
void packet_handler(u_char *param, const struct pcap_pkthdr *header, const u_char *pkt_data)
{
#if 0
    struct tm *ltime;
    char timestr[16];
    time_t local_tv_sec;

    /* 将时间戳转换成可识别的格式 */
    local_tv_sec = header->ts.tv_sec;
    ltime=localtime(&local_tv_sec);
    strftime(timestr, sizeof timestr, "%H:%M:%S", ltime);
    fprintf(stderr, "%s,%.6d len:%d\n", timestr, header->ts.tv_usec, header->len);
#endif

    int i = rand() % gqueue_num;
    en_queue(&pkt_queue[i], (char *)pkt_data + 14, header->len);

}

/*
 * func:    work thread to deal with the coming data 
 * param:   
 * return:  
 */
DWORD WINAPI worker_thread(void *dev)
{
    if(dev == NULL)
    {
        return -1;
    } 
    pcap_if_t *d = (pcap_if_t *)dev;
    pcap_t *adhandle;

    /* 打开设备 */
    if ( (adhandle= pcap_open(d->name,          // 设备名
                    65536,            // 65535保证能捕获到不同数据链路层上的每个数据包的全部内容
                    PCAP_OPENFLAG_PROMISCUOUS,    // 混杂模式
                    1000,             // 读取超时时间
                    NULL,             // 远程机器验证
                    errbuf            // 错误缓冲池
                    ) ) == NULL)
    {
        fprintf(gfp_log, "\nUnable to open the adapter. %s is not supported by WinPcap\n", d->name);
        fflush(gfp_log);
        /* 释放设备列表 */
        pcap_freealldevs(alldevs);
        return -1;
    }

    fprintf(gfp_log, "\nlistening on %s...\n", d->description);
    fprintf(gfp_log, "filter condition: %s\n", gbpf_filter_string); 
    fflush(gfp_log);

    if(InterlockedIncrement(&finish_flag) >= count_dev)
    {
        pcap_freealldevs(alldevs);
        //fprintf(stderr, "free the dev %d %d!\n", finish_flag, count_dev); 
    }

    bpf_u_int32 net_mask = PCAP_NETMASK_UNKNOWN;
    struct bpf_program bpf_filter;  //BPF过滤规则

    int ret = pcap_compile(adhandle, &bpf_filter, gbpf_filter_string, 0, net_mask); //编译过滤规则   
    if(ret < 0)
    {
        fprintf(gfp_log, "pcap_compile error:%s", pcap_geterr(adhandle));
        fflush(gfp_log);
    }

    ret = pcap_setfilter(adhandle, &bpf_filter);//设置过滤规则
    if(ret < 0)
    {
        fprintf(gfp_log, "pcap_setfilter error:%s", pcap_geterr(adhandle));
        fflush(gfp_log);
    }
    /* 开始捕获 */
    ret = pcap_loop(adhandle, 0, packet_handler, NULL);
    if(ret < 0)
    {
        fprintf(gfp_log, "pcap_loop error:%s", pcap_geterr(adhandle));
        fflush(gfp_log);
    }
}

/*
 * func:    work thread to deal with the coming data 
 * param:   
 * return:  
 */
DWORD WINAPI handle_work(void *param)
{
    int queue_id = *(int *)param;
    free(param);
#define PKT_BUF_SIZE 2048
    char buf[PKT_BUF_SIZE]; 
    int buf_len;
    while(1)
    {
        if(de_queue(&pkt_queue[queue_id], buf, &buf_len) == 0)
        {
            HandleIncomingData((unsigned char *)buf, buf_len, 0);
        }
        else
        {
            Sleep(100);
        }
    }
}
