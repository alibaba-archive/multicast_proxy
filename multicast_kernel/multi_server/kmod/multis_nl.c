#include <linux/ip.h>
#include <net/ip.h>
#include <net/tcp.h>
#include <net/udp.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <linux/version.h>
#include <linux/module.h>

//#include "tmc_control_pkt.h"
#include "tmcc_nl.h"
#include "grp.h"

static struct sock * multi_nl_sk;

int multi_nl_node_add(struct tmcc_msg_st *msg_st)
{
    int ret;
    struct tmcc_nl_service_st *service;

    service = (struct tmcc_nl_service_st *)msg_st->data;

    /* let vm join into multicast group */
    if(service->action == MULTI_OPT_JOIN)
    {
        ret = add_multi_node(service->multi_ip,service->ip_list, service->ip_num);
        if(ret)
        {
            service->ret = ret;
            return ret;
        }
    }

    /* let vm quit from multicast group */
    if(service->action == MULTI_OPT_QUIT)
    {
        ret = vm_quit_from_multi_grp(service->multi_ip,service->ip_list, service->ip_num);
        if(ret)
        {
            service->ret = ret;
            return ret;
        }
    }
	service->ret = 0;
    return 0;
}

int multi_nl_node_delete(struct tmcc_msg_st *msg_st)
{
    struct tmcc_nl_service_st *service;
    int ret;

    service = (struct tmcc_nl_service_st *)msg_st->data;
    ret = del_multi_node(service->multi_ip);
    if(ret)
    {
        return ret;
    }

    return 0;
}

int multi_nl_node_clear(struct tmcc_msg_st *msg_st)
{
    clear_multi_group();
    return 0;
}

static struct tmcc_nl_show_service_st  k_service;
int multi_nl_vm_list_get(struct tmcc_msg_st *msg_st)
{
    struct tmcc_nl_show_service_st __user *u_service;
    struct tmcc_nl_service_st  *service_in_kernel;
    int ret;

    service_in_kernel = (struct tmcc_nl_service_st *)msg_st->data;

    u_service = (struct tmcc_nl_show_service_st __user *)msg_st->reply_ptr;

    k_service.ip_num = 0;

    //printk(KERN_ERR "multi_ip %04x\n",service_in_kernel->multi_ip);

    ret = list_multi_node_and_vm_ip(service_in_kernel->multi_ip,&k_service);
	//printk(KERN_ERR "get result %d\n",ret);
    if(ret)
    {
        u_service->ret = ret;
        return ret;
    }


    if(copy_to_user(u_service, &k_service, sizeof(struct tmcc_nl_show_service_st)))
    {
        return -1;
    }

	u_service->ret = 0;
    return 0;

}

int multi_nl_grp_show(struct tmcc_msg_st *msg_st)
{
    struct tmcc_nl_show_service_st __user *u_service;
    int ret;

    u_service = (struct tmcc_nl_show_service_st __user *)msg_st->reply_ptr;

    k_service.ip_num = 0;

    ret = get_all_multi_grp(&k_service);
    if(ret)
    {
        u_service->ret = ret;
        return -1;
    }

    if(copy_to_user(u_service, &k_service, sizeof(struct tmcc_nl_show_service_st)))
    {
        return -1;
    }

	u_service->ret = 0;
    return 0;

}

int multi_nl_node_get_drop_stats(struct tmcc_msg_st *msg_st)
{
    struct multi_server_drop_stats __user *u_stats;
    struct multi_server_drop_stats k_stats;
    struct tmcc_nl_service_st  *service_in_kernel;

    u_stats = (struct multi_server_drop_stats __user *)msg_st->reply_ptr;
    service_in_kernel = (struct tmcc_nl_service_st *)msg_st->data;

    memset(&k_stats, 0, sizeof(struct multi_server_drop_stats));

    if(service_in_kernel->multi_ip == 0)
        get_all_multi_drop_pkt_stats(&k_stats);
    else
        get_multi_pkt_stats(service_in_kernel->multi_ip, &k_stats);

    if(copy_to_user(u_stats, &k_stats, sizeof(struct multi_server_drop_stats)))
    {
        return -1;
    }

    return 0;
}


int multi_nl_vm_list_append(struct tmcc_msg_st *msg_st)
{
    struct tmcc_nl_service_st *service;
    int ret;

    service = (struct tmcc_nl_service_st *)msg_st->data;
    ret = append_vm_ip_list(service->multi_ip,service->ip_list,service->ip_num);
    if(ret)
    {
        service->ret = ret;
        return -1;
    }

	service->ret = 0;
	return 0;
}

void tmcc_rcv_skb(struct sk_buff *skb)
{
    int type, pid, flags, nlmsglen, skblen, ret = TMCC_OK;
    struct 	nlmsghdr *nlh;
    struct tmcc_msg_st *msg_st;

    skblen = skb->len;
    if (skblen < sizeof(*nlh)){
        return;
    }

#if  LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,18)
    nlh = (struct nlmsghdr *)skb->data;
#else
    nlh = nlmsg_hdr(skb);
#endif
    nlmsglen = nlh->nlmsg_len;
    if (nlmsglen < sizeof(*nlh) || skblen < nlmsglen){
        return;
    }

    pid = nlh->nlmsg_pid;
    flags = nlh->nlmsg_flags;

    if(flags & MSG_TRUNC){
        return;
    }

    type = nlh->nlmsg_type;

    msg_st = (struct tmcc_msg_st *)nlh;

    switch(type){
        case TMCC_SERVICE_ADD:
            ret = multi_nl_node_add(msg_st);
            break;
        case TMCC_SERVICE_DEL:
            ret = multi_nl_node_delete(msg_st);
            break;
        case MULTI_SERVER_CLEAR:
            ret = multi_nl_node_clear(msg_st);
            break;
        case TMCC_SERVICE_CHECK_UPDATE:
            ret = multi_nl_vm_list_append(msg_st);
            break;
        case MULTI_DROP_STATS:
            ret = multi_nl_node_get_drop_stats(msg_st);
            break;
        case TMCC_SERVICE_LIST:
            ret = multi_nl_vm_list_get(msg_st);
            break;
        case TMCC_SERVICE_SHOW:
            ret = multi_nl_grp_show(msg_st);
            break;
        default:
            break;
    }

    netlink_ack(skb, nlh, ret);
    return;
}

#if  LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,18)
static DEFINE_MUTEX(rx_queue_mutex);

static void tmcc_rcv_sock(struct sock *sk, int len)
{
    struct sk_buff *skb;

    mutex_lock(&rx_queue_mutex);

    while ((skb = skb_dequeue(&sk->sk_receive_queue)) != NULL) {
        tmcc_rcv_skb(skb);
        kfree_skb(skb);
    }

    mutex_unlock(&rx_queue_mutex);
}
#endif

int multi_nl_init(void)
{
    /*netlink varies greatly in different kernel version, so in fact we only cover 2.6.32 and 3.10 kernel version.*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
    struct netlink_kernel_cfg cfg = {
        .input  = tmcc_rcv_skb,
    };
    multi_nl_sk = netlink_kernel_create(&init_net, MULTI_NL, &cfg);
#elif LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,18)
    multi_nl_sk = netlink_kernel_create(MULTI_NL, 0, tmcc_rcv_sock, THIS_MODULE);
#else
    multi_nl_sk = netlink_kernel_create(&init_net, MULTI_NL, 0, tmcc_rcv_skb, NULL, THIS_MODULE);
#endif
    if(multi_nl_sk == NULL){
        printk(KERN_ERR "tmcc_rcv_skb: failed to create netlink socket\n");
        return -1;
    }

    return 0;
}

void multi_nl_fini(void)
{
#if  LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,18)
    sock_release(multi_nl_sk->sk_socket);
#else
    netlink_kernel_release(multi_nl_sk);
#endif
}
