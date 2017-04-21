#include <linux/ip.h>
#include <net/ip.h>
#include <net/tcp.h>
#include <net/udp.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include<linux/timer.h>
#include<linux/jhash.h>
#include<linux/jiffies.h>

#include "tmcc_nl.h"
#include "grp.h"


struct multi_node multi_grp[MULTI_GRP_BUCKET][MULTI_GRP_DEPTH];

static atomic_t node_cnt = ATOMIC_INIT(0);

static spinlock_t   multi_lock;



static uint32_t ip_port_hash_key(uint32_t server_ip , uint16_t port,  uint32_t maxnum)
{
    return jhash_2words(server_ip,port, 0) % maxnum;
}

struct multi_node *__lookup_ip_port_node(uint32_t server_ip, uint16_t port, uint32_t *index,uint32_t *row_id)
{
    uint32_t idx;
    int i;

    idx = ip_port_hash_key(server_ip, port,  MULTI_GRP_BUCKET); 
    *index = idx;

    for(i = 0; i < MULTI_GRP_DEPTH; i++)
    {
        if(multi_grp[idx][i].created == 0) //no grp later in this bucket
            break;
        
        if(multi_grp[idx][i].server_ip== server_ip && multi_grp[idx][i].port == port)
        {
            *row_id = i;
            return &multi_grp[idx][i];
        }
    }

    *row_id = 0;
    return NULL;
}



int lookup_ip_port_node(uint32_t server_ip, uint16_t port, uint32_t *multi_ip, uint32_t *index, uint32_t *row_id)
{
    struct multi_node *node;

    spin_lock_bh(&multi_lock);

    node = __lookup_ip_port_node(server_ip, port, index, row_id);
    if(node == NULL)
    {
        spin_unlock_bh(&multi_lock);
        return -1;
    }

    *multi_ip = node->multi_ip;

    spin_unlock_bh(&multi_lock);

    return 0;
}


int list_all_multi_grp(struct tmcc_nl_show_service_st  *k_service)
{
    uint32_t i,j,k=0;

    spin_lock_bh(&multi_lock);

    for(i = 0; i < MULTI_GRP_BUCKET; i++)
    {
        for(j = 0; j < MULTI_GRP_DEPTH; j++)
        {
            if(multi_grp[i][j].created == 0)
                break;
            else
            {
                k_service->multi_ip[k] = multi_grp[i][j].multi_ip;
                k_service->port[k] = multi_grp[i][j].port;
                k_service->server_ip[k] = multi_grp[i][j].server_ip;
				k++;
            }
        }
     }
     k_service->node_cnt = k;

     spin_unlock_bh(&multi_lock);
     
     return 0;
}

int add_multi_node(uint32_t multi_ip,uint32_t server_ip, uint16_t port)
{
    uint32_t index, row_id;
    uint8_t i;
    struct multi_node *node = NULL;
    int ret = 0;

    if(atomic_read(&node_cnt) >= MULTI_GRP_MAX)
        return MULTI_TBL_FULL;
	
    spin_lock_bh(&multi_lock);

    node = __lookup_ip_port_node(server_ip, port, &index, &row_id);
    if(node != NULL)
    {
        /* multi node exsit, we try update it */
        
        if(node->multi_ip == multi_ip)
        {
            ret = MULTI_NODE_EXSIT;
            goto FINISH;
        }

        node->multi_ip = multi_ip;
        
        goto FINISH;
    }

    /* multi node not exsit */
    
    for(i = 0; i < MULTI_GRP_DEPTH; i++)
    {
        node = &multi_grp[index][i];
        if(node->created == 0)
        {    //an unuse location
            node->multi_ip = multi_ip;
            node->server_ip= server_ip;
            node->port = port;
            node->created = 1;
            node->rx_packets = 0;
            node->rx_bytes = 0;
            atomic_inc(&node_cnt);
            goto FINISH;
        }
    }

    ret = MULTI_IDX_EXHAUST;

FINISH:
    spin_unlock_bh(&multi_lock);
    return ret;
}





int del_multi_node(uint32_t server_ip, uint16_t port)
{
    uint32_t index, row_id;
    int j;
    struct multi_node *node_del = NULL;
    struct multi_node *node_move = NULL;    
    int ret = 0;

    spin_lock_bh(&multi_lock);

    node_del = __lookup_ip_port_node(server_ip, port, &index, &row_id);
    if(node_del == NULL)
    {
        ret = MULTI_NODE_NOT_EXSIT;
        goto FINISH;
    }

    /* judge if hash node exsit after node_del */
    for(j = MULTI_GRP_DEPTH-1; j >= 0; j--)
    {
        if(row_id == j)
        {
            j = -1;
            break;
        }
        node_move = &multi_grp[index][j];
        if(node_move->created == 1)
            break;
    }

    if(j >= 0)/* if node exsit, need move to this delete node position */
    {
        node_del->multi_ip = node_move->multi_ip;
        node_del->server_ip= node_move->server_ip;
        node_del->port = node_move->port;
        node_del->created = node_move->created;
        
        node_move->created = 0;
        node_move->multi_ip = 0;
        node_move->server_ip = 0;
        node_move->port = 0;

        atomic_dec(&node_cnt);
    }
    else /* if node not exsit, we only delete this node */
    {
        node_del->created = 0;
        node_del->server_ip = 0;
        node_del->multi_ip = 0;
        node_del->port = 0;
        atomic_dec(&node_cnt);
    }
    
FINISH:
    spin_unlock_bh(&multi_lock);
    return ret;
}

int clear_multi_node(void)
{
    uint16_t i;

    spin_lock_bh(&multi_lock);

    for(i=0; i<MULTI_GRP_BUCKET; i++){
        memset(multi_grp[i], 0, sizeof(struct multi_node) * MULTI_GRP_DEPTH);
    }

    atomic_set(&node_cnt, 0);

    spin_unlock_bh(&multi_lock);

    return 0;
}

void multi_client_stats(uint32_t multi_ip, uint32_t bucket, uint32_t depth, uint16_t len)
{

    spin_lock_bh(&multi_lock);

    if(multi_ip == multi_grp[bucket][depth].multi_ip){
         multi_grp[bucket][depth].rx_packets++;
         multi_grp[bucket][depth].rx_bytes += len;
    }

    spin_unlock_bh(&multi_lock);
}

void get_multi_packets_stats(struct multic_packets_stats_st *stats, uint32_t ip, uint16_t port)
{
    uint32_t index, row_id;
    struct multi_node *node = NULL;

    spin_lock_bh(&multi_lock);

    node = __lookup_ip_port_node(ip, port, &index, &row_id);
    if(node != NULL){
        stats->rx_packets = node->rx_packets;
        stats->rx_bytes = node->rx_bytes;
        stats->ret = 0;
    }
    else
        stats->ret = 1;

    spin_unlock_bh(&multi_lock);

    return;
}

int multi_node_init(void)
{
    spin_lock_init(&multi_lock);

    memset(multi_grp, 0, sizeof(struct multi_node) * MULTI_GRP_BUCKET * MULTI_GRP_DEPTH);

    return 0;
}

void multi_node_fini(void)
{
    return;
}
