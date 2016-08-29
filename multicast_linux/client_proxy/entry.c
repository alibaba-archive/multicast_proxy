#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/if_tun.h>
#include <net/if.h>
#include "connectInit.h"
#include "config.h"
#include "handleData.h"
#include "worker_thread.h"
#include "macro_define.h"
#include "dev_pcap.h"

char tun_name[IFNAMSIZ];
int tun;

int tun_create(char *dev, int flags)
{
    struct ifreq ifr;
    int fd, err;
    assert(dev != NULL);
    if ((fd = open("/dev/net/tun", O_RDWR)) < 0)
        return fd;

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags |= flags;
    if (*dev != '\0')
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) {
        close(fd);
        return err;
    }
    strcpy(dev, ifr.ifr_name);
    return fd;
}

int entry()
{

    //Initialize
    log_init(CLIENT_LOG_PATH);
    multi_node_init(); 
    int rt = cfg_init(CLIENT_CFG);
    if(rt != 0)
    {
        fprintf(gfp_log, "[%s:%d]cfg_init error!\n", __FILE__, __LINE__);
        fflush(gfp_log);
        return -1;
    }
    rt = dev_name_init();
    if(rt != 0)
    {
        fprintf(gfp_log, "[%s:%d]dev_name_init error\n",__FILE__,__LINE__);
        fflush(gfp_log);
        return -1;
    }
    tun = tun_create(tun_name, IFF_TUN | IFF_NO_PI);
    if(tun < 0)
    {
        fprintf(gfp_log, "[%s:%d]tun dev open error\n", __FILE__,__LINE__);
        fflush(gfp_log);
        return -1;
    }
    printf("tun_name: %s\n", tun_name);
    pthread_t preload_t;
    int err  = pthread_create(&preload_t,NULL,cfg_reload,NULL);
    if(err)
    {
         fprintf(gfp_log,"[%s:%d]Create reload Thread Failed\n", __FILE__, __LINE__);
         fflush(gfp_log);
         return -1;
    }
    if (!CreateNetConnections ())
    {
        fprintf(gfp_log, "[%s:%d]Error condition @ CreateNetConnections  , exiting\n", __FILE__, __LINE__);
        fflush(gfp_log);
        return 1;
    }

    int worker_thread_num = 2 * sysconf(_SC_NPROCESSORS_CONF); 
//    int worker_thread_num = 1; 
    if (CreateWorkers (worker_thread_num) != 0)
    {
        fprintf(gfp_log, "[%s:%d]Error condition @CreateWorkers, exiting\n", __FILE__, __LINE__);
        fflush(gfp_log);
        return 1;
    }
    pthread_join(preload_t,NULL);
    log_uninit();
    return 0;
}
/*int main(int argc , char ** argv)
{
     entry();
     printf("entry.c conpilor success");
     return 1;
}*/
