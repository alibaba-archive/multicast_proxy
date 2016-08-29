#include <stdlib.h>
#include <string.h>
#include "multi_grp.h"

#define BITS_PER_BYTE 8

struct multi_node multi_grp[MULTI_GRP_BUCKET][MULTI_GRP_DEPTH];
uint32_t multi_vm[MULTI_GRP_MAX][MULTI_VM_MAX];
//struct multi_stats_st multi_stats[MULTI_GRP_MAX];

unsigned long g_bitmap;

/*
 * func:    test and set to 1 for the nr bit of addr 
 *          and return the origin data in the nr bit 
 * param:   nr is the bit of seting; addr is the data to be setted.
 * return:  0 when the nr bit in addr is 0; 1 when it is 1 
 */
static inline int multi_test_and_set_bit(uint16_t nr, unsigned long *addr)
{
	if((((1U << (nr % BITS_PER_BYTE)) & ((uint8_t *)addr)[nr / BITS_PER_BYTE])) != 0)
	{
		((uint8_t *)addr)[nr / BITS_PER_BYTE] |= (1U << (nr % BITS_PER_BYTE));
		return 1;
	}
	else
	{
		((uint8_t *)addr)[nr / BITS_PER_BYTE] |= (1U << (nr % BITS_PER_BYTE));
		return 0;
	}
}

/*
 * func:    clear the nr bit of addr 
 * param:   nr is the bit to be cleared; addr is the data to be setted.
 * return:  
 */
static inline void multi_clear_bit(uint16_t nr, unsigned long *addr)
{
    ((uint8_t *)addr)[nr / BITS_PER_BYTE] &= ~(1U << (nr % BITS_PER_BYTE));
}

/*
 * func:    put a idx into bitmap pool
 * param:   idx is the bit to be cleared; addr is the data to be setted.
 * return:  
 */
int put_idx(unsigned long *base_addr, uint32_t idx)
{
    multi_clear_bit(idx, base_addr);
    return 0;
}

/*
 * func:    get a idx from bitmap pool
 * param:   idx is the bit to be getted; addr is the data.
 * return:  0 success; -1 fail. 
 */
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

/*
 * func:    
 * param:   
 * return:   
 */
int multi_node_init(void)
{
	memset(multi_grp, 0, sizeof(struct multi_node) * MULTI_GRP_BUCKET * MULTI_GRP_DEPTH);
	memset(multi_vm, 0, sizeof(uint32_t) * MULTI_GRP_MAX * MULTI_VM_MAX);
    //memset(multi_stats, 0, sizeof(struct multi_stats_st) * MULTI_GRP_MAX);
    g_bitmap = 0;

	return 0;
}

/*
 * func:    
 * param:   
 * return:   
 */
static uint32_t mulit_hash_key(uint32_t multi_ip, uint16_t multi_port, uint32_t maxnum)
{
    return (multi_ip + multi_port) * 1023 % maxnum;
}

/*
 * func:    
 * param:   (index, row_id) decide the location in multi_grp
 * return:  the location of the pointer of struct
 */
struct multi_node *__lookup_multi_node(uint32_t mulit_ip, uint16_t multi_port, uint32_t *index,uint32_t *row_id)
{
    uint32_t idx;
    int i;

    idx = mulit_hash_key(mulit_ip, multi_port, MULTI_GRP_BUCKET); 
    *index = idx;

    for(i = 0; i < MULTI_GRP_DEPTH; i++)
    {   
        if(multi_grp[idx][i].created == 0) //no group later in this bucket
            break;
        if(multi_grp[idx][i].multi_ip == mulit_ip && multi_grp[idx][i].multi_port == multi_port)
        {   
            *row_id = i;
            return &multi_grp[idx][i];
        }   
    }   

    *row_id = 0;
    return NULL;
}

/*
 * func:    
 * param:   
 * return:  
 */
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

/*
 * func:    
 * param:   
 * return:  
 */
int add_multi_node(uint32_t multi_ip, 
				uint16_t multi_port, 
				uint16_t server_port, 
				uint32_t *ip_list, 
				uint8_t ip_num)
{
    uint32_t index, row_id;
    uint8_t i;
    struct multi_node *node = NULL;
    uint32_t idx;
    int ret = 0;

    //spin_lock_bh(&multi_lock);

    node = __lookup_multi_node(multi_ip, multi_port, &index, &row_id);
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
    ret = get_idx(&g_bitmap, &idx);
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
            
            node->multi_port = multi_port;
            node->server_port = server_port;

            //multi_stats[idx].tx_packets = 0;
            //multi_stats[idx].tx_bytes = 0;

            ret = add_vm_ip_list(idx, ip_list, ip_num, &(node->multi_member_cnt));
            if(ret)
            {
                ret = MULTI_PARAM_ERROR;
            }
            goto FINISH;
        }
    }

    //add group fail because of hash table collision
    put_idx(&g_bitmap, idx);

    ret = MULTI_IDX_EXHAUST;

FINISH:
    //spin_unlock_bh(&multi_lock);
    return ret;
}

/*
 * func:    
 * param:   
 * return:  
 */
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

/*
 * func:    
 * param:   
 * return:  
 */
int lookup_multi_node(uint32_t multi_ip, uint16_t multi_port, uint32_t iplist[], struct multi_node *node)
{
    uint32_t index, row_id;   
    struct multi_node * tmp;  

    //spin_lock_bh(&multi_lock);

    tmp = __lookup_multi_node(multi_ip, multi_port, &index, &row_id); 
    if(tmp == NULL)
    {  
        //spin_unlock_bh(&multi_lock);   
        return -1;
    }  

    node->multi_member_cnt = tmp->multi_member_cnt;
    node->multi_grp_idx = tmp->multi_grp_idx;
    node->server_port = tmp->server_port;
    get_vm_ip_list(node->multi_grp_idx, iplist, node->multi_member_cnt);

    //spin_unlock_bh(&multi_lock); 
    return 0;
}
