#ifndef __MULTI_H__
#define __MULTI_H__

struct multi_node
{
    uint32_t multi_ip;
    uint32_t server_ip;
    uint16_t port;
    uint8_t created;

    uint64_t rx_packets;
    uint64_t rx_bytes;
};

struct multi_key
{
    uint32_t 	multi_ip;
}__attribute__((__packed__));


#define     MULTI_GRP_BUCKET     64
#define     MULTI_GRP_DEPTH	    8
#define	 MULTI_GRP_MAX	          64 
#define     MULTI_VM_MAX            128


extern struct multi_node multi_grp[MULTI_GRP_BUCKET][MULTI_GRP_DEPTH];


struct tmcc_msg_st{
    struct  nlmsghdr hdr;
    void    *reply_ptr;
    char    data[0];
};

struct  tmcc_reply_st{
    struct nlmsghdr     hdr;
    struct nlmsgerr		err;
};

struct tmcc_nl_service_st
{
    int ret;
    uint16_t port;
    uint32_t server_ip;    
    uint32_t multi_ip;
};

struct tmcc_nl_show_service_st
{
    int ret;
    uint8_t node_cnt;
    uint16_t port[MULTI_GRP_MAX];
    uint32_t server_ip[MULTI_GRP_MAX];    
    uint32_t multi_ip[MULTI_GRP_MAX];
};

struct multic_packets_stats_st{
    uint32_t ret;
    uint64_t rx_bytes;
    uint64_t rx_packets;
};

enum
{
    MULTI_NODE_EXSIT = 1000,
    MULTI_NODE_NOT_EXSIT,
    MULTI_IDX_EXHAUST,
    MULTI_PARAM_ERROR,
    MULTI_TBL_FULL,
    RPINT_ERROR
};

extern int multi_node_init(void);
extern void multi_node_fini(void);

extern int add_multi_node(uint32_t multi_ip,uint32_t server_ip, uint16_t port);
extern int del_multi_node(uint32_t server_ip, uint16_t port);
extern int clear_multi_node(void);
extern int list_all_multi_grp(struct tmcc_nl_show_service_st  *k_service);
//extern struct multi_node *lookup_ip_port_node(uint32_t server_ip, uint16_t port);
extern int lookup_ip_port_node(uint32_t server_ip, uint16_t port, uint32_t *multi_ip, uint32_t *index, uint32_t *row_id);
extern void multi_client_stats(uint32_t multi_ip, uint32_t bucket, uint32_t depth, uint16_t len);
extern void get_all_multi_packets_stats(struct multic_packets_stats_st *stats); 
extern void get_multi_packets_stats(struct multic_packets_stats_st *stats, uint32_t ip, uint16_t port);

#define PRINT_IP_FORMAT         "%u.%u.%u.%u"
#define  PRINT_HIP(x)\
        ((x >> 24) & 0xFF),\
        ((x >> 16) & 0xFF),\
        ((x >>  8) & 0xFF),\
        ((x >>  0) & 0xFF)

#endif
