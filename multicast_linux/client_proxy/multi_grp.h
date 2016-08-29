/**********************************
 * the parameter all is host order.
 **********************************/


#ifndef __MULTI_GRP_H__
#define __MULTI_GRP_H__
#include <stdio.h>
#include <stdlib.h>

typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

struct multi_node
{
    uint32_t multi_ip;
    uint32_t server_ip; 
	uint16_t multi_port;
    uint16_t server_port;
    uint8_t created;			//exist or not
};

struct multi_key
{
    uint32_t    multi_ip;
}__attribute__((__packed__));


#define  MULTI_GRP_BUCKET    1024 //64
#define  MULTI_GRP_DEPTH     256
#define  MULTI_GRP_MAX       256 //16
#define  MULTI_VM_MAX        128

enum
{
    MULTI_NODE_EXSIT = 1000,
    MULTI_NODE_NOT_EXSIT,
    MULTI_IDX_EXHAUST,
    MULTI_PARAM_ERROR,
    MULTI_VM_LAGER_THAN_MAX
    
};


extern struct multi_node multi_grp[MULTI_GRP_BUCKET][MULTI_GRP_DEPTH];

int multi_node_init(void);
struct multi_node *__lookup_ip_port_node(uint32_t server_ip, uint16_t port, uint32_t *index,uint32_t *row_id);
int add_multi_node(uint32_t multi_ip, 
				uint16_t multi_port, 
				uint32_t server_ip, 
				uint16_t server_port);
int lookup_ip_port_node(uint32_t server_ip, 
						uint16_t server_port, 
						uint32_t *multi_ip, 
						uint16_t *multi_port, 
						uint32_t *index, 
						uint32_t *row_id);				
#endif
