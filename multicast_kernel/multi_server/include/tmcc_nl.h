#ifndef __TMCC_H__
#define __TMCC_H__

#include "grp.h"

#define TMCC_MSG_TIMEOUT 5
#define MULTI_NL 29

enum {
    TMCC_OK = 0,
    TMCC_SERVICE_EXIST = 1000,
    TMCC_SERVICE_NOT_EXIST,
    TMCC_SERVICE_FULL,
    TMCC_POLICY_TABLE_FULL,
	TMCC_CMD_PARAMS_NULL,
	TMCC_CMD_ADDR_TRANS_ERROR,
	TMCC_CMD_U32_TRANS_ERROR,
	TMCC_CMD_OPTION_ERROR,
    TMCC_COPY_TO_USER_FAIL,
    TMCC_MEMORY_ALLOC_FAIL,
    TMCC_IP_LIST_TOO_LONG,
    TMCC_CREATE_PROC_ENTRY_FAIL,
    TMCC_ALLOC_SKB_FAIL,    

    TMCC_GATEWAY_EXIST,
    TMCC_GATEWAY_NOT_EXIST,
    TMCC_GATEWAY_FULL,
    TMCC_GET_GW_FAIL,

    TMCC_SESSION_COUNT_TOO_MUCH
};

enum{
    MULTI_OPT_JOIN=1,
    MULTI_OPT_QUIT
};

enum{
    TMCC_SERVICE_ADD = 0,
    TMCC_SERVICE_DEL,
    MULTI_DROP_STATS,
    MULTI_SERVER_CLEAR,
    TMCC_SERVICE_INFO_LIST,
    TMCC_SERVICE_LIST,
    TMCC_SERVICE_SHOW,
    TMCC_SERVICE_CHECK_UPDATE,
    TMCC_SERVICE_UPDATE,

    TMCC_GW_ADD,
    TMCC_GW_DEL,
	TMCC_GW_CHECK_UPDATE,
    TMCC_ENABLE_BFD,
    TMCC_DISABLE_BFD,
    TMCC_GW_GET,
    TMCC_GW_LIST,
    TMCC_PKT_STATS_OVERVIEW,
    TMCC_PKT_STATS_GW,

    TMCC_SESSION_LIST,
    TMCC_SESSION_CLEAR,

	TMCC_SWITCH,
	TMCC_SWITCH_LIST
};


#ifdef __KERNEL__
#else
#include <linux/netlink.h>

int tmcc_service_add_to_kernel(struct tmcc_nl_service_st *service, uint32_t datalen);
int tmcc_service_delete_from_kernel(struct tmcc_nl_service_st *service, uint32_t datalen);
int tmcc_service_check_update_from_kernel(void);
int tmcc_service_list(struct tmcc_nl_service_st *serv, uint32_t datalen);
int tmcc_service_show(struct tmcc_nl_service_st *serv, uint32_t datalen);
int multi_drop_stats(struct tmcc_nl_service_st *serv, uint32_t datalen);
int multi_server_clear_from_kernel(void);
int multi_server_config_clear(void);


#endif

#endif
