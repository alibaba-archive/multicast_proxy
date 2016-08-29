#ifndef __MULTI_H__
#define __MULTI_H__
#include <stdio.h>
#include <stdlib.h>

typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

struct multi_node
{
    uint32_t multi_ip;
    uint32_t multi_member_cnt;
    uint8_t multi_grp_idx;
    uint8_t created;
    
    uint16_t multi_port;
    uint16_t server_port;
};

struct multi_key
{
    uint32_t    multi_ip;
}__attribute__((__packed__));

#define  MULTI_GRP_MAX       256
#define  MULTI_VM_MAX        128

#define  MULTI_GRP_BUCKET    512
#define  MULTI_GRP_DEPTH     MULTI_GRP_MAX

enum
{
    MULTI_NODE_EXSIT = 1000,
    MULTI_NODE_NOT_EXSIT,
    MULTI_IDX_EXHAUST,
    MULTI_PARAM_ERROR,
    MULTI_VM_LAGER_THAN_MAX
    
};


extern struct multi_node multi_grp[MULTI_GRP_BUCKET][MULTI_GRP_DEPTH];

extern uint32_t multi_vm[MULTI_GRP_MAX][MULTI_VM_MAX];


int multi_node_init(void);
struct multi_node *__lookup_multi_node(uint32_t mulit_ip, uint16_t multi_port, uint32_t *index,uint32_t *row_id);

int add_multi_node(uint32_t multi_ip, 
				uint16_t multi_port, 
				uint16_t server_port, 
				uint32_t *ip_list, 
				uint8_t ip_num);
				
int lookup_multi_node(uint32_t multi_ip, uint16_t multi_port, uint32_t iplist[], struct multi_node *node);

#endif
