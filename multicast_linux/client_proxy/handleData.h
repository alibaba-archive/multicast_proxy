#ifndef _HANDLE_DATA_H_
#define _HANDLE_DATA_H_ 

#define HAVE_REMOTE
#include <pcap.h>
#include "multi_grp.h"
#include "macro_define.h"
#include "log.h"
#include "iphdr.h"
#define true 1
#define false 0


extern unsigned int grecv_pkt;
extern unsigned int gdrop_pkt;
extern unsigned int gforward_pkt;
int restruct_pkt(char *buf,
        struct eth_hdr *pethh,
        struct ip_hdr *piph, int iph_len,
        struct udp_hdr *pudph,
        uint32_t multi_ip,
        uint16_t multi_port);
int restruct_pkt_others_proto(char *buf,
        struct eth_hdr *pethh,
        struct ip_hdr *piph, int iph_len,
        struct udp_hdr *pudph,
        uint32_t multi_ip,
        uint16_t multi_port);
int HandleIncomingData(unsigned char * pBuf, int bufLen);

#endif
