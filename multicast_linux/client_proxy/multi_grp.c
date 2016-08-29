#include <stdlib.h>
#include <string.h>
#include "multi_grp.h"

struct multi_node multi_grp[MULTI_GRP_BUCKET][MULTI_GRP_DEPTH];

/*
 * func:	
 * param:   
 * return:  
 */
int multi_node_init(void)
{
    memset(multi_grp, 0, sizeof(struct multi_node) * MULTI_GRP_BUCKET * MULTI_GRP_DEPTH);

    return 0;
}

/*
 * func:	
 * param:   
 * return:  
 */
static uint32_t ip_port_hash_key(uint32_t server_ip, uint16_t port, uint32_t maxnum)
{
    return (server_ip % port) * 1023 % maxnum;
}

/*
 * func:  lookup the node by ip and port
 * param£º(index, row_id) decide the location in multi_grp
 * return: the location of the pointer of struct
 */
struct multi_node *__lookup_ip_port_node(uint32_t server_ip, uint16_t port, uint32_t *index,uint32_t *row_id)
{
    uint32_t idx;
    int i;

    idx = ip_port_hash_key(server_ip, port, MULTI_GRP_BUCKET); 
    *index = idx;

    for(i = 0; i < MULTI_GRP_DEPTH; i++)
    {   
        if(multi_grp[idx][i].created == 0) //no group later in this bucket
            break;
        if(multi_grp[idx][i].server_ip == server_ip && multi_grp[idx][i].server_port == port)
        {   
            *row_id = i;
            return &multi_grp[idx][i];
        }
    }

    *row_id = 0;
    return NULL;
}

/*
 * func:  	add the multi_ip and multi_port and so on to the hash table.
 * param:	multi_ip: the group ip of multicast.
multi_port: the group port of multicast.
server_ip: the source ip of multicast.
server_port: the dest port of multicast. 
 * return:	MULTI_NODE_EXSIT: the node exsits.
MULTI_IDX_EXHAUST: the hash table is full.
0: success. 
 */
int add_multi_node(uint32_t multi_ip, 
        uint16_t multi_port, 
        uint32_t server_ip, 
        uint16_t server_port)
{
    uint32_t index, row_id;
    uint8_t i;
    struct multi_node *node = NULL;
    int ret = 0;

    //if(atomic_read(&node_cnt) >= MULTI_GRP_MAX)
    //   return MULTI_TBL_FULL;

    //spin_lock_bh(&multi_lock);

    node = __lookup_ip_port_node(server_ip, server_port, &index, &row_id);
    if(node != NULL)
    {   
        /* multi node exsit, we try update it */

        if(node->multi_ip == multi_ip)
        {   
            ret = MULTI_NODE_EXSIT;
            goto FINISH;
        }   

        node->multi_ip = multi_ip;
        node->multi_port = multi_port;

        goto FINISH;
    }   

    /* multi node not exsit */

    for(i = 0; i < MULTI_GRP_DEPTH; i++)
    {
        node = &multi_grp[index][i];
        if(node->created == 0)
        {    //an unuse location
            node->multi_ip = multi_ip;
            node->multi_port = multi_port;
            node->server_ip= server_ip;
            node->server_port = server_port;
            node->created = 1;
            //node->rx_packets = 0;
            //node->rx_bytes = 0;
            //atomic_inc(&node_cnt);
            goto FINISH;
        }
    }

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
int lookup_ip_port_node(uint32_t server_ip, 
        uint16_t server_port, 
        uint32_t *multi_ip, 
        uint16_t *multi_port, 
        uint32_t *index, 
        uint32_t *row_id)                                                                                         
{
    struct multi_node *node;

    //spin_lock_bh(&multi_lock);

    node = __lookup_ip_port_node(server_ip, server_port, index, row_id);
    if(node == NULL)
    {
        //spin_unlock_bh(&multi_lock);
        return -1;
    }

    *multi_ip = node->multi_ip;
    *multi_port = node->multi_port;
    //spin_unlock_bh(&multi_lock);

    return 0;
}
