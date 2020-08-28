#ifndef __MULTI_H__
#define __MULTI_H__


struct multi_node
{
    uint32_t multi_ip;
    uint32_t multi_member_cnt;
    uint8_t multi_grp_idx;
    uint8_t created;
};

struct multi_key
{
    uint32_t 	multi_ip;
}__attribute__((__packed__));


#define     MULTI_GRP_BUCKET    1024
#define     MULTI_GRP_DEPTH     512
#define     MULTI_GRP_MAX       1024
#define     MULTI_VM_MAX        1024


extern struct multi_node multi_grp[MULTI_GRP_BUCKET][MULTI_GRP_DEPTH];

extern uint32_t multi_vm[MULTI_GRP_MAX][MULTI_VM_MAX]; 


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
    uint8_t action;
    uint32_t multi_ip;
    uint32_t ip_num;
    uint32_t ip_list[MULTI_VM_MAX];
};

struct tmcc_nl_show_service_st{
    int ret;
    uint32_t multi_ip;
    uint32_t ip_num;
    uint32_t ip_list[MULTI_VM_MAX];
};

struct multi_server_drop_stats{
    uint8_t ret;
    uint64_t tx_packets;
    uint64_t tx_bytes;
    uint64_t no_multi_grp_pkt_drop_count;
    uint64_t no_multi_member_pkt_drop_count; 
    uint64_t route_fail_drop_count;
    uint64_t no_mem_drop_count;
};

struct multi_stats_st{
    uint64_t tx_packets;
    uint64_t tx_bytes;
};

enum
{
    MULTI_NODE_EXSIT = 1000,
    MULTI_NODE_NOT_EXSIT,
    MULTI_IDX_EXHAUST,
    MULTI_PARAM_ERROR,
    MULTI_VM_LAGER_THAN_MAX
    
};

extern int multi_node_init(void);
extern void multi_node_fini(void);
extern int lookup_multi_node(uint32_t mulit_ip, uint32_t iplist[], struct multi_node *node);
extern int add_multi_node(uint32_t multi_ip,uint32_t *ip_list, uint32_t ip_num);
extern int delete_vm_from_multi_grp(uint32_t multi_ip,uint32_t *ip_list, uint32_t ip_num);
extern int append_vm_ip_list(uint32_t multi_ip,uint32_t *ip_list, uint32_t ip_num);
extern int del_multi_node(uint32_t multi_ip);
extern int get_vm_ip_list(uint8_t idx, uint32_t *ip_list, uint32_t ip_num);
extern int get_multi_node_ip_list(uint32_t multi_ip, struct tmcc_nl_show_service_st *get_vm_ip);
extern int get_all_multi_grp(struct tmcc_nl_show_service_st  *k_service);
extern int vm_quit_from_multi_grp(uint32_t multi_ip,uint32_t *ip_list, uint32_t ip_num);
extern int list_multi_node_and_vm_ip(uint32_t multi_ip, struct tmcc_nl_show_service_st *get_vm_ip);
extern void get_all_multi_drop_pkt_stats(struct multi_server_drop_stats *k_stats);
extern void multi_grp_stats(uint16_t grp_idx, uint16_t len); 
extern void get_multi_pkt_stats(uint32_t multi_ip, struct multi_server_drop_stats *k_stats); 
extern void clear_multi_group(void); 

#define PRINT_IP_FORMAT         "%u.%u.%u.%u"
#define  PRINT_HIP(x)\
        ((x >> 24) & 0xFF),\
        ((x >> 16) & 0xFF),\
        ((x >>  8) & 0xFF),\
        ((x >>  0) & 0xFF)

#endif
