#include <linux/ip.h>
#include <net/ip.h>
#include <net/tcp.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/version.h>
#include <linux/module.h>

#include "tmcc_nl.h"
#include "grp.h"

static uint64_t rx_packets = 0;
static uint64_t rx_bytes = 0;

void get_all_multi_packets_stats(struct multic_packets_stats_st *stats)
{
    stats->rx_packets =  rx_packets;
    stats->rx_bytes =  rx_bytes;
    return;
}

#if  LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,18)

unsigned int tmcc_hook_local_in(unsigned int hook,
        struct sk_buff **pskb,
        const struct net_device *in,
        const struct net_device *out,
        int (*okfn)(struct sk_buff *))
#elif LINUX_VERSION_CODE <= KERNEL_VERSION(4,0,0)

unsigned int tmcc_hook_local_in(const struct nf_hook_ops *ops,
        struct sk_buff *skb,
        const struct net_device *in,
        const struct net_device *out,
    #ifndef __GENKSYMS__
        const struct nf_hook_state *state
    #else
        int (*okfn)(struct sk_buff *)
    #endif
        )
#elif LINUX_VERSION_CODE <= KERNEL_VERSION(4,2,0)

static unsigned int tmcc_hook_local_in(const struct nf_hook_ops *ops,
        struct sk_buff *skb,
        const struct nf_hook_state *state)
#else

static unsigned int tmcc_hook_local_in(void *priv,
        struct sk_buff *skb,
        const struct nf_hook_state *state)
#endif
{

#if  LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,18)
    struct sk_buff *skb = *pskb;
#endif
   // struct multi_node * node = NULL;
    struct iphdr    *iph = ip_hdr(skb);
    struct udphdr *uh = NULL;
    struct udphdr uth;
    uint8_t protocol = iph->protocol;
    uint32_t saddr = ntohl(iph->saddr);
    uint32_t multi_ip, bucket, depth;
    uint16_t port;
    uint32_t old_addr;
    int ret;
   
    if(protocol != IPPROTO_UDP)
        return NF_ACCEPT;
    
    //printk(KERN_ERR "Got a multicast packet, %04x %04x", saddr, iph->saddr);

    uh = skb_header_pointer(skb, ip_hdrlen(skb), sizeof(struct udphdr), &uth);
    if(uh == NULL)
        return NF_ACCEPT;

    port = htons(uh->dest); 

    //printk(KERN_ERR "before lookup port  %d %04x",port, port);

    ret = lookup_ip_port_node(saddr, port, &multi_ip, &bucket, &depth);
    if(ret != 0)
    {
        ret = lookup_ip_port_node(saddr, 0, &multi_ip, &bucket, &depth);
        if(ret != 0) {
            return NF_ACCEPT;
        }
    }

    old_addr = iph->daddr;
    iph->daddr = htonl(multi_ip);
    /*
    iph->check = 0;
    iph->check = ip_fast_csum((__u8 *) iph, iph->ihl);
    uh->check = 0;
    uh->check = csum_tcpudp_magic(iph->saddr, iph->daddr, ntohs(iph->tot_len)-iph->ihl * 4,
                                  IPPROTO_UDP,csum_partial((void *)uh, ntohs(iph->tot_len)-iph->ihl * 4, 0));
	skb->ip_summed = CHECKSUM_UNNECESSARY;
    */
    if(!(iph->frag_off & htons(IP_OFFSET))){
        csum_replace4(&iph->check, old_addr, iph->daddr);
        if(uh->check || skb->ip_summed == CHECKSUM_PARTIAL){
            inet_proto_csum_replace4(&uh->check, skb, old_addr, iph->daddr, 1);
            if(!uh->check)
                uh->check = CSUM_MANGLED_0;
        }
    }
	//printk(KERN_ERR "Get a node, multi ip is %04x %04x",node->multi_ip, iph->daddr);
    //statistic packets
    rx_packets++;
    rx_bytes += ntohs(iph->tot_len);
    multi_client_stats(multi_ip, bucket, depth, ntohs(iph->tot_len));
    //node->rx_packets++;
    //node->rx_bytes += ntohs(iph->tot_len);
    
    return NF_ACCEPT;
}

static struct nf_hook_ops multi_ops[] __read_mostly = {
    {
        .hook           = tmcc_hook_local_in,

#if  LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,18)
        .owner          = THIS_MODULE,
        .pf             = PF_INET,
        .hooknum        = NF_IP_PRE_ROUTING,
#elif LINUX_VERSION_CODE <= KERNEL_VERSION(4,2,0)
        .owner          = THIS_MODULE,
        .pf             = NFPROTO_IPV4,
        .hooknum        = NF_INET_PRE_ROUTING,
#else
        .pf             = NFPROTO_IPV4,
        .hooknum        = NF_INET_PRE_ROUTING,
#endif

        .priority       = NF_IP_PRI_FIRST,
    }
};

int multi_nf_init(void)
{
    if(nf_register_hooks(multi_ops, ARRAY_SIZE(multi_ops)) != 0){
        printk(KERN_ERR "register hook in netfilter failure\n");
        return -1;
    }

    return 0;
}

void multi_nf_fini(void)
{
    nf_unregister_hooks(multi_ops, ARRAY_SIZE(multi_ops));
}

