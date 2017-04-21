#include <linux/ip.h>
#include <net/ip.h>
#include <net/tcp.h>
#include <net/udp.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include<linux/jhash.h>
#include<linux/timer.h>
#include<linux/jiffies.h>

#include "tmcc_nl.h"
//#include "tmc_control_pkt.h"
#include "grp.h"


struct multi_node multi_grp[MULTI_GRP_BUCKET][MULTI_GRP_DEPTH];

uint32_t multi_vm[MULTI_GRP_MAX][MULTI_VM_MAX]; 
struct multi_stats_st multi_stats[MULTI_GRP_MAX];

unsigned long bitmap;

static spinlock_t   multi_lock;


static inline int multi_test_bit(uint32_t nr, const unsigned long *addr)
{
//    return (((1U << (nr % BITS_PER_BYTE)) & ((uint8_t *)addr)[nr / BITS_PER_BYTE])) != 0;
    return test_bit(nr, addr);
}

static inline void multi_set_bit(uint16_t nr, unsigned long *addr)
{
    //((uint8_t *)addr)[nr / BITS_PER_BYTE] |= 1U << (nr % BITS_PER_BYTE);
    set_bit(nr, addr);
}

static inline void multi_clear_bit(uint16_t nr, unsigned long *addr)
{
    //((uint8_t *)addr)[nr / BITS_PER_BYTE] &= ~(1U << (nr % BITS_PER_BYTE));
    clear_bit(nr, addr);
}

static inline int multi_test_and_set_bit(uint16_t nr, unsigned long *addr)
{
    //((uint8_t *)addr)[nr / BITS_PER_BYTE] &= ~(1U << (nr % BITS_PER_BYTE));
    return test_and_set_bit(nr, addr);
}


/* get a idx from bitmap pool */
int get_idx(unsigned long *base_addr, uint32_t *idx)
{
    uint32_t i;
    for(i = 0; i < MULTI_GRP_MAX; i++)
    {
        if(multi_test_and_set_bit(i, base_addr) == 0)
        {
            *idx = i;
            return 0;
        }
    }
    return -1;
}

/* put a idx into bitmap pool */
int put_idx(unsigned long *base_addr, uint32_t idx)
{
    multi_clear_bit(idx, base_addr);
    return 0;
}



static uint32_t mulit_hash_key(uint32_t multi_ip , uint32_t maxnum)
{
    return jhash_1word(multi_ip, 0) % maxnum;
}


struct multi_node *__lookup_multi_node(uint32_t mulit_ip, uint32_t *index,uint32_t *row_id)
{
    uint32_t idx;
    int i;

    idx = mulit_hash_key(mulit_ip, MULTI_GRP_BUCKET); 
    *index = idx;

    for(i = 0; i < MULTI_GRP_DEPTH; i++)
    {
        if(multi_grp[idx][i].created == 0) //no group later in this bucket
            break;
        if(multi_grp[idx][i].multi_ip == mulit_ip)
        {
            *row_id = i;
            return &multi_grp[idx][i];
        }
    }

    *row_id = 0;
    return NULL;
}


int lookup_multi_node(uint32_t multi_ip, uint32_t iplist[], struct multi_node *node)
{
    uint32_t index, row_id;
    struct multi_node * tmp;

    spin_lock_bh(&multi_lock);

    tmp = __lookup_multi_node(multi_ip, &index, &row_id);
    if(tmp == NULL)
    {
        spin_unlock_bh(&multi_lock);
        return -1;
    }

    node->multi_member_cnt = tmp->multi_member_cnt;
    node->multi_grp_idx = tmp->multi_grp_idx;
    get_vm_ip_list(node->multi_grp_idx, iplist, node->multi_member_cnt);

    spin_unlock_bh(&multi_lock);

    return 0;
}


static int add_vm_ip_list(uint8_t idx, uint32_t *ip_list, uint32_t ip_num, uint32_t *old_ip_num)
{
    int i,j,k;
    
    k = *old_ip_num;
    if(ip_list == NULL || ip_num + (*old_ip_num) > MULTI_VM_MAX)
        return MULTI_PARAM_ERROR;
    
    for(i = 0; i < ip_num; i++)
    {
        for(j = 0; j < *old_ip_num; j++)
        {
            /* new vm ip not exsit in multi_vm group, we fetch next vm ip in multi_vm group */
            if(multi_vm[idx][j] != ip_list[i])
                continue;
            else /* if new vm ip has already exsit in multi_vm group, we break for loop to fetch next new ip */
                break;
        }
        /* new vm ip exsit, fetch next new vm ip */
        if(j < *old_ip_num)
            continue;
        else/* new vm ip not exsit, add into multi_vm group */
        {
            multi_vm[idx][k] = ip_list[i];
            k++;
        }
    }
    
    *old_ip_num = k;
    return 0;
}



int get_vm_ip_list(uint8_t idx, uint32_t *ip_list, uint8_t ip_num)
{
    int i;
    
    if(ip_list == NULL)
        return MULTI_PARAM_ERROR;
    
    for(i = 0; i < ip_num; i++)
    {
        ip_list[i] = multi_vm[idx][i];
    }
        return 0;
}


static void clear_vm_ip_list(uint8_t idx)
{
    int i;
    for(i = 0; i < MULTI_VM_MAX; i++)
    {
        multi_vm[idx][i] = 0;
    }
}



int append_vm_ip_list(uint32_t multi_ip,uint32_t *ip_list, uint8_t ip_num)
{
    uint32_t index, row_idx;
    struct multi_node *node = NULL;
    uint8_t idx;
    int ret = 0;

    spin_lock_bh(&multi_lock);

    node = __lookup_multi_node(multi_ip, &index, &row_idx);
    if(node == NULL)
    {
        ret = MULTI_NODE_NOT_EXSIT;
        goto FINISH;
    } 

    idx = node->multi_grp_idx;
    ret = add_vm_ip_list(idx, ip_list, ip_num, &(node->multi_member_cnt));
    if(ret)
    {
        ret =  MULTI_PARAM_ERROR;
        goto FINISH;
    }

FINISH:
    spin_unlock_bh(&multi_lock);
    return ret;
}

int get_all_multi_grp(struct tmcc_nl_show_service_st  *k_service)
{
    uint32_t i,j,k=0;

    for(i = 0; i < MULTI_GRP_BUCKET; i++)
    {
        for(j = 0; j < MULTI_GRP_DEPTH; j++)
        {
            if(multi_grp[i][j].created == 0)
                break;
            else
                k_service->ip_list[k++] = multi_grp[i][j].multi_ip;
        }
    }
    k_service->ip_num = k;
     
    return 0;
}


int list_multi_node_and_vm_ip(uint32_t multi_ip, struct tmcc_nl_show_service_st *get_vm_ip)
{
    uint32_t index, row_id;
    struct multi_node *node = NULL;
    uint8_t idx;
    int ret = 0;

    spin_lock_bh(&multi_lock);

    node = __lookup_multi_node(multi_ip, &index, &row_id);
    if(node == NULL)
    {
        ret = MULTI_NODE_NOT_EXSIT;
        goto FINISH;
    } 
    
    idx = node->multi_grp_idx;
    get_vm_ip->ip_num = node->multi_member_cnt;
    get_vm_ip->multi_ip = node->multi_ip;

//    printk(KERN_ERR "idx %u ip_num %u multi_ip %04x\n",idx, get_vm_ip->ip_num, get_vm_ip->multi_ip);
    
    ret = get_vm_ip_list(idx, get_vm_ip->ip_list, node->multi_member_cnt);
    if(ret)
    {
        ret = MULTI_PARAM_ERROR;
        goto FINISH;
    }
    
FINISH:
    spin_unlock_bh(&multi_lock);
    return ret;
}


static void del_quit_vm_ip(uint32_t idx, uint32_t *ip_list, uint8_t ip_num, uint32_t *old_ip_num)
{
    int i, j = 0;
	uint32_t last_ip = *old_ip_num;

    for(i = 0; i < ip_num; i++)
    {
        for(j = 0; j < *old_ip_num; j++)
        {
            if(multi_vm[idx][j] == ip_list[i])
            {
                //multi_vm[idx][j] = 0;
				multi_vm[idx][j] = multi_vm[idx][last_ip - 1];
				multi_vm[idx][last_ip - 1] = 0;
				last_ip--;
				*old_ip_num = last_ip;
                break;
            }
        }
    }
}

int vm_quit_from_multi_grp(uint32_t multi_ip,uint32_t *ip_list, uint8_t ip_num)
{
    int ret = 0;
    uint32_t index, row_id;
    struct multi_node *node = NULL;

    spin_lock_bh(&multi_lock);

    node = __lookup_multi_node(multi_ip, &index, &row_id);
    if(node == NULL)
    {
        ret = MULTI_NODE_NOT_EXSIT;
        goto FINISH;
    }
    
    del_quit_vm_ip(node->multi_grp_idx, ip_list, ip_num, &(node->multi_member_cnt));

FINISH:
    spin_unlock_bh(&multi_lock);
    
    return ret;
}

int add_multi_node(uint32_t multi_ip,uint32_t *ip_list, uint8_t ip_num)
{
    uint32_t index, row_id;
    uint8_t i;
    struct multi_node *node = NULL;
    uint32_t idx;
    int ret = 0;

    spin_lock_bh(&multi_lock);

    node = __lookup_multi_node(multi_ip, &index, &row_id);
    if(node != NULL)
    {
        /* multi node exsit, we append vm ip into vm ip list */
        ret = add_vm_ip_list(node->multi_grp_idx, ip_list, ip_num, &(node->multi_member_cnt));
        if(ret)
        {
            ret = MULTI_PARAM_ERROR;
        }
        goto FINISH;
    }

    /* multi node not exsit, we add all vm ip into vm ip list */
   // printk(KERN_ERR "after node is NULL,index %u",index);
    ret = get_idx(&bitmap, &idx);
    if(ret)
    {
        ret = MULTI_IDX_EXHAUST;
        goto FINISH;
    }

    
    for(i = 0; i < MULTI_GRP_DEPTH; i++)
    {
        node = &multi_grp[index][i];
        if(node->created == 0){    //an unuse location
            node->multi_ip = multi_ip;
            node->multi_grp_idx = idx;
            node->multi_member_cnt = 0;
            node->created = 1;

            multi_stats[idx].tx_packets = 0;
            multi_stats[idx].tx_bytes = 0;

            ret = add_vm_ip_list(idx, ip_list, ip_num, &(node->multi_member_cnt));
            if(ret)
            {
                ret = MULTI_PARAM_ERROR;
            }
            goto FINISH;   
        }
    }

    //add group fail because of hash table collision
    put_idx(&bitmap, idx);

    ret = MULTI_IDX_EXHAUST;

FINISH:
    spin_unlock_bh(&multi_lock);
    return ret;
}

int del_multi_node(uint32_t multi_ip)
{
    uint32_t index, row_id;
    uint8_t idx;
    int j;
    struct multi_node *node_del = NULL;
    struct multi_node *node_move = NULL;    
    int ret = 0;

    spin_lock_bh(&multi_lock);

    node_del = __lookup_multi_node(multi_ip, &index, &row_id);
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

    if(j >= 0)/* node exsit, need move to this delete node position */
    {
        idx = node_del->multi_grp_idx;
        node_del->multi_member_cnt = node_move->multi_member_cnt;
        node_del->created = node_move->created;
        node_del->multi_ip = node_move->multi_ip;
        node_del->multi_grp_idx = node_move->multi_grp_idx;

        node_move->created = 0;
        node_move->multi_member_cnt = 0;
        node_move->multi_ip = 0;
        node_move->multi_grp_idx = 0;

        clear_vm_ip_list(idx);
        ret = put_idx(&bitmap, idx);

    }
    else /* later node not exsit, we only delete this node */
    {
        idx = node_del->multi_grp_idx;
        node_del->created = 0;
        node_del->multi_member_cnt = 0;
        node_del->multi_ip = 0;
        node_del->multi_grp_idx = 0;

        clear_vm_ip_list(idx);
        ret = put_idx(&bitmap, idx);
    }

FINISH:
    spin_unlock_bh(&multi_lock);

    return 0;
}

void multi_grp_stats(uint16_t grp_idx, uint16_t len)
{
    multi_stats[grp_idx].tx_packets++;
    multi_stats[grp_idx].tx_bytes += len;
    return;
}

void get_multi_pkt_stats(uint32_t multi_ip, struct multi_server_drop_stats *k_stats)
{
    uint32_t index, row_id;
    struct multi_node *node = NULL;

    spin_lock_bh(&multi_lock);

    node = __lookup_multi_node(multi_ip, &index, &row_id);
    if(node != NULL){
        k_stats->tx_packets = multi_stats[node->multi_grp_idx].tx_packets;
        k_stats->tx_bytes = multi_stats[node->multi_grp_idx].tx_bytes;
        k_stats->ret = 0;
    }
    else
        k_stats->ret = 1;

    spin_unlock_bh(&multi_lock);
}

void clear_multi_group(void)
{
    uint16_t i, j;

    spin_lock_bh(&multi_lock);

    for(i = 0; i < MULTI_GRP_BUCKET; i++){
        for(j = 0; j < MULTI_GRP_DEPTH; j++){
            if(multi_grp[i][j].created == 0)
                break;

            multi_clear_bit(multi_grp[i][j].multi_grp_idx, &bitmap);
            memset(&multi_stats[multi_grp[i][j].multi_grp_idx], 0, sizeof(struct multi_stats_st));
            clear_vm_ip_list(multi_grp[i][j].multi_grp_idx);
            memset(&multi_grp[i][j], 0, sizeof(struct multi_node));
        }
    }

    spin_unlock_bh(&multi_lock);
}


int multi_node_init(void)
{
    spin_lock_init(&multi_lock);

    memset(multi_grp, 0, sizeof(struct multi_node) * MULTI_GRP_BUCKET * MULTI_GRP_DEPTH);

    memset(multi_vm, 0, sizeof(uint32_t) * MULTI_GRP_MAX * MULTI_VM_MAX); 
    memset(multi_stats, 0, sizeof(struct multi_stats_st) * MULTI_GRP_MAX);

    bitmap = 0;

    return 0;
}

void multi_node_fini(void)
{
    return;
}
