#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "iphdr.h"
#include "log.h"

u_int16_t checksum(u_int16_t * addr, int len)
{
    int     nleft = len;
    u_int32_t sum = 0;
    u_int16_t *w = addr;
    u_int16_t answer = 0;

    /*
     * Our algorithm is simple, using a 32 bit accumulator (sum), we add
     * sequential 16 bit words to it, and at the end, fold back all the
     * carry bits from the top 16 bits into the lower 16 bits.
     */
    while (nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }
    /* mop up an odd byte, if necessary */
    if (nleft == 1) {
        * (unsigned char *) (&answer) = * (unsigned char *) w;
        sum += answer;
    }

    /* add back carry outs from top 16 bits to low 16 bits */
    sum = (sum >> 16) + (sum & 0xffff); /* add hi 16 to low 16 */
    sum += (sum >> 16);     /* add carry */
    answer = ~sum;     /* truncate to 16 bits */
    return (answer);
}

unsigned short udp_checksum(struct ip_hdr *iph, int iph_len, int udp_len)
{
    char *buf;
    int udp_psdh_size = sizeof(struct udp_psd_header);
    int udp_size = udp_psdh_size + udp_len;
    buf = (char *)malloc(udp_size);
    if(buf == NULL)
    {
        fprintf(gfp_log, "[%s:%d]malloc in udp_checksum\n", __FILE__, __LINE__);
        return -1;
    }
    memset(buf, 0, udp_size);

    struct udp_hdr *udph = (struct udp_hdr *)((unsigned char *)iph + iph_len);

    struct udp_psd_header *pudp_psd_header = (struct udp_psd_header *)buf;
    pudp_psd_header->src_addr = iph->ip_srcaddr;
    pudp_psd_header->dst_addr = iph->ip_destaddr;
    pudp_psd_header->mbz = 0;
    pudp_psd_header->ptcl = IPPROTO_UDP;
    pudp_psd_header->udplen = udph->udp_length;

    memcpy(buf + udp_psdh_size, (unsigned char *)udph, udp_len);

    unsigned short csum = checksum((unsigned short *)buf, udp_size);
    free(buf);

    return csum;
}
