#include <stdio.h> 
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "iphdr.h"
#include "config.h"
#include "multi_grp.h"
#include "worker_thread.h"
#include "connectInit.h"
#include "macro_define.h"

unsigned int groupIp[256];
int groupNum;

int  entry()
{
    //Initialize
    //log_init(SERVER_LOG_PATH);
    log_init(SERVER_LOG_PATH);
    multi_ip_max_min_init();
    multi_node_init();

    int rt = cfg_init(SERVER_CFG, groupIp, &groupNum);
    if(rt!= 0)
    {
        fprintf(gfp_log, "[%s:%d]cfg_init error!\n", __FILE__, __LINE__);
        return -1;
    } 

    //create thread to reload cfg
    pthread_t terminal_t;
    int err = pthread_create (&terminal_t, NULL, terminal_command, NULL);
    if (err)
    {
        fprintf (gfp_log, "[%s:%d]Create terminal_command Thread Failed\n", __FILE__, __LINE__);
        fflush(gfp_log);
        return -1;
    }

    //
    // try to get timing more accurate... Avoid context
    // switch that could occur when threads are released
    //


    if (!CreateNetConnections (groupIp, groupNum))
    {
        fprintf(gfp_log, "[%s:%d]Error condition @ CreateNetConnections  , exiting\n", __FILE__, __LINE__);
        return -1;
    }
    int worker_thread_num = 2 * sysconf(_SC_NPROCESSORS_CONF); 
//    worker_thread_num =1;
    if (CreateWorkers(worker_thread_num) != 0)
    {
        fprintf(gfp_log, "[%s:%d]Error condition @CreateWorkers, exiting\n", __FILE__, __LINE__);
        return -1;
    }
    pthread_join(terminal_t , NULL);
    log_uninit();
    return 0;
}
/*int main(int argc,char ** argv)
{
    entry();
    return 1;
}*/
