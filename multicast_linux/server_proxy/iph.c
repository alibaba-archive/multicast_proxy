#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "iphdr.h"
#include "log.h"

unsigned short checksum(unsigned short *buf, int size)
{
    unsigned long cksum = 0;
    while(size > 1)
    {
        cksum += *buf++;
        size -= sizeof(unsigned short);
    }
    if(size)
    {
        cksum += *(unsigned char *)buf;
    }

    while(cksum >> 16)
        cksum = (cksum >> 16) + (cksum & 0xffff);

    return (unsigned short)(~cksum);
}

unsigned short udp_checksum(void *iph, int iph_len, int udp_len)
{
    struct udp_psd_header *pudp_psd_header;
    int v_udphead_size = sizeof(struct udp_psd_header);
    int udp_size = v_udphead_size + udp_len;
    pudp_psd_header = (struct udp_psd_header *)malloc(udp_size);

    if(pudp_psd_header == NULL)
    {
        fprintf(gfp_log, "[%s:%d]malloc in udp_checksum\n", __FILE__, __LINE__);
        return -1;
    }
    memset(pudp_psd_header, 0, udp_size);

    pudp_psd_header->src_addr = ((struct ip_hdr *)iph)->ip_srcaddr;
    pudp_psd_header->dst_addr = ((struct ip_hdr *)iph)->ip_destaddr;
    pudp_psd_header->udplen = htons(udp_len);
    pudp_psd_header->mbz = 0;
    pudp_psd_header->ptcl = 17;//IPPROTO_UDP

    memcpy((char *)pudp_psd_header + v_udphead_size, (unsigned char *)iph + iph_len, udp_len);

    unsigned short csum = checksum((unsigned short *)pudp_psd_header, udp_size);
    free(pudp_psd_header);
    return csum;
}
