#ifdef _MSC_VER
/*
 * we do not want the warnings about the old deprecated and unsecure CRT functions
 * since these examples can be compiled under *nix as well
 */
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include "multi_grp.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <pcap.h>
#include "log.h"

pcap_if_t *alldevs;
char errbuf[PCAP_ERRBUF_SIZE];

int dev_name_init()
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
/*
 * func:  get the dev name by ip
 * param£º
 * return: success NOT NULL; fail NULL
 */
char *get_dev_name(uint32_t ip, char *dev_name)
{
    pcap_if_t *d;

    if(dev_name == NULL)
    {
        fprintf(gfp_log, "[%s:%d]dev_name is null\n", __FILE__, __LINE__);
        fflush(gfp_log);
        return NULL;
    }
    /* Print the list */
    for(d=alldevs; d; d=d->next)
    {
        pcap_addr_t *a;
        for(a=d->addresses; a; a=a->next)
        {
            switch(a->addr->sa_family)
            {
                case AF_INET:
                    if(a->addr)
                        if(((struct sockaddr_in *)(a->addr))->sin_addr.s_addr
                                == ip)
                        {
                            /*
                            int name_len = strlen(d->name);
                            int i, j=0;
                            for(i=0; i<name_len; i++)
                            {
                                dev_name[j++] = d->name[i];
                                if(d->name[i] == '\\')
                                {
                                    dev_name[j++] = '\\';
                                }
                            }
                            dev_name[j] = 0;
                            //printf("%s", dev_name);
                            return dev_name;
                            */
                            strcpy(dev_name, d->name);
                            return dev_name;
                        }
                    break;
                default:
                    ;//printf("Address family name: unknow!\n\n");
            } 
        }
    }
    return NULL; 
}
