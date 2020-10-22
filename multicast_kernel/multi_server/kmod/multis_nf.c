#include <linux/ip.h>
#include <net/ip.h>
#include <net/tcp.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <linux/netfilter_ipv4.h>
#include <linux/version.h>
#include <linux/module.h>

//#include "tmc_control_pkt.h"
#include "tmcc_nl.h"
#include "grp.h"


static uint64_t no_multi_grp_pkt_drop_count = 0;
static uint64_t no_vm_ip_list_pkt_drop_count = 0;
static uint64_t route_fail_drop_count = 0;
static uint64_t no_mem_drop_count = 0;
static uint64_t tx_packets = 0;
static uint64_t tx_bytes = 0;

void get_all_multi_drop_pkt_stats(struct multi_server_drop_stats *k_stats)
{
    k_stats->no_multi_grp_pkt_drop_count = no_multi_grp_pkt_drop_count;
    k_stats->no_multi_member_pkt_drop_count = no_vm_ip_list_pkt_drop_count;
    k_stats->route_fail_drop_count = route_fail_drop_count;
    k_stats->no_mem_drop_count = no_mem_drop_count;
    k_stats->tx_packets = tx_packets;
    k_stats->tx_bytes = tx_bytes;
    return;
}

#if  LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,18)
static inline bool ipv4_is_multicast(__be32 addr)
{
        return (addr & htonl(0xf0000000)) == htonl(0xe0000000);
}
#endif

#if  LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,18)

unsigned int tmcs_hook_local_out(unsigned int hook,
        struct sk_buff **pskb,
        const struct net_device *in,
        const struct net_device *out,
        int (*okfn)(struct sk_buff *))
#elif LINUX_VERSION_CODE <= KERNEL_VERSION(4,0,0)

unsigned int tmcs_hook_local_out(const struct nf_hook_ops *ops,
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

unsigned int tmcs_hook_local_out(const struct nf_hook_ops *ops,
        struct sk_buff *skb,
        const struct nf_hook_state *state)
#else
static unsigned int tmcs_hook_local_out(void *priv,
        struct sk_buff *skb,
        const struct nf_hook_state *state)

#endif
{
#if  LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,18)
    struct sk_buff *skb = *pskb;
#endif
    int i, ret;
    struct multi_node node;
    uint32_t vm_ip_list[MULTI_VM_MAX];
    struct sk_buff *new_skb = NULL;
    struct iphdr    *iph = ip_hdr(skb);
    struct udphdr *uh = NULL;
    struct udphdr uth;
	uint32_t daddr = htonl(iph->daddr);
    uint8_t protocol = iph->protocol;
    uint32_t old_addr;

    if(protocol != IPPROTO_UDP)
        return NF_ACCEPT;

    //if(!ipv4_is_multicast(iph->daddr))
    //    return NF_ACCEPT;

    //printk(KERN_ERR "Got a multicast packet, %04x!",daddr);

    ret = lookup_multi_node(daddr, vm_ip_list, &node);
    if(ret != 0)
    {
        no_multi_grp_pkt_drop_count++;
        return NF_ACCEPT;
    }

    //printk(KERN_ERR "Get a node, vm counter is %d",node->multi_member_cnt);

    if(node.multi_member_cnt == 0)
    {
        no_vm_ip_list_pkt_drop_count++;
        return NF_ACCEPT;
    }

    // printk(KERN_ERR "%d\n", node->multi_member_cnt);
    for(i = 0; i < node.multi_member_cnt; i++)
    {
        new_skb = skb_copy(skb, GFP_ATOMIC);

        if (unlikely(new_skb == NULL)){
            no_mem_drop_count++;
            continue;
        }

        //printk(KERN_ERR "vm_ip %04x\n",vm_ip_list[i]);
        iph = ip_hdr(new_skb);
	    //iph->ttl = 64;  //need ?
	    uh = skb_header_pointer(new_skb, ip_hdrlen(new_skb), sizeof(struct udphdr), &uth);
        if(uh == NULL)
            continue;

		if(iph->ttl < 64)
			iph->ttl = 64;

        iph->tos = 0xE0;

        old_addr = iph->daddr;
        iph->daddr = htonl(vm_ip_list[i]);
        /*
        iph->check = 0 ;
        iph->check = ip_fast_csum((__u8 *) iph, iph->ihl);
	    uh->check = 0;
        uh->check = csum_tcpudp_magic(iph->saddr, iph->daddr, ntohs(iph->tot_len)-iph->ihl * 4,
                      IPPROTO_UDP,csum_partial((void *)uh, ntohs(iph->tot_len)-iph->ihl * 4, 0));
        */
        if(!(iph->frag_off & htons(IP_OFFSET))){
            csum_replace4(&iph->check, old_addr, iph->daddr);
            if(uh->check || new_skb->ip_summed == CHECKSUM_PARTIAL){
                inet_proto_csum_replace4(&uh->check, new_skb, old_addr, iph->daddr, 1);
                if(!uh->check)
                    uh->check = CSUM_MANGLED_0;
            }
        }

		//new_skb->ip_summed = CHECKSUM_UNNECESSARY;
        //reroute because of dnat
#if  LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,18)
        if(ip_route_me_harder(&new_skb) != 0){
#elif LINUX_VERSION_CODE <= KERNEL_VERSION(4,2,0)
        if(ip_route_me_harder(new_skb, RTN_UNSPEC) != 0){
#else
        if(ip_route_me_harder(state->net, new_skb, RTN_UNSPEC) != 0){
#endif
            kfree_skb(new_skb);
            route_fail_drop_count++;
            continue;
        }

#if  LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,18)
        skb->dst->output(new_skb);
#elif LINUX_VERSION_CODE <= KERNEL_VERSION(4,2,0)
        ip_local_out(new_skb);
#else
        ip_local_out(dev_net(skb_dst(skb)->dev), new_skb->sk, new_skb);
#endif

        //statistic
        tx_packets++;
        tx_bytes += ntohs(iph->tot_len);
        multi_grp_stats(node.multi_grp_idx, ntohs(iph->tot_len));
    }

    return NF_ACCEPT;
}

static struct nf_hook_ops multi_ops[] __read_mostly = {
    {
        .hook           = tmcs_hook_local_out,
#if  LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,18)
        .owner          = THIS_MODULE,
        .pf             = PF_INET,
        .hooknum        = NF_IP_LOCAL_OUT,
#elif LINUX_VERSION_CODE <= KERNEL_VERSION(4,2,0)
        .owner          = THIS_MODULE,
        .pf             = NFPROTO_IPV4,
        .hooknum        = NF_INET_LOCAL_OUT,
#else
        .pf             = NFPROTO_IPV4,
        .hooknum        = NF_INET_LOCAL_OUT,
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

