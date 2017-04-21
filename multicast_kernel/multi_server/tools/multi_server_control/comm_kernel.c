#include <unistd.h>
#include <stdio.h>
#include <linux/types.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include "base_control.h"
//#include "tmc_control_pkt.h"
#include "tmcc_nl.h"

static int skfd;

int tmcc_service_add_to_kernel(struct tmcc_nl_service_st *serv, uint32_t datalen)
{
    uint32_t msg_len;
	struct tmcc_msg_st *message;

	tmcc_comm_kernel_prepare();

    msg_len = NLMSG_LENGTH(datalen + sizeof(void *));
    message = (struct tmcc_msg_st *)malloc(msg_len);
    if(message == NULL){
		fprintf(stderr, "mem allocate failure!\n");
		close(skfd);
		return -1;
	}

	memset(message, 0, msg_len);	
	message->hdr.nlmsg_flags = NLM_F_REQUEST;
	message->hdr.nlmsg_pid = local.nl_pid;		
	message->hdr.nlmsg_len = msg_len;
	message->hdr.nlmsg_type = TMCC_SERVICE_ADD;
	memcpy(message->data, serv, datalen);

	tmcc_comm_kernel_finish();
	free(message);
	
	return reply.err.error;
}

int tmcc_service_delete_from_kernel(struct tmcc_nl_service_st *serv, uint32_t datalen)
{
    uint32_t msg_len;
    struct tmcc_msg_st *message;

	tmcc_comm_kernel_prepare();

    msg_len = NLMSG_LENGTH(datalen + sizeof(void *));
    message = (struct tmcc_msg_st *)malloc(msg_len);
    if(message == NULL){
		fprintf(stderr, "mem allocate failure!\n");
		close(skfd);
		return -1;
	}

	memset(message, 0, msg_len);	
	message->hdr.nlmsg_flags = NLM_F_REQUEST;
	message->hdr.nlmsg_pid = local.nl_pid;		
	message->hdr.nlmsg_len = msg_len;
	message->hdr.nlmsg_type = TMCC_SERVICE_DEL;
	memcpy(message->data, serv, datalen);

	tmcc_comm_kernel_finish();
	free(message);
	
	return reply.err.error;
}

void print_error(char *action, int errno);

void print_vm_ip_info(struct tmcc_nl_show_service_st *service)
{
    uint32_t i;

	//printf("service-ret %u %u\n",service->ret,MULTI_NODE_NOT_EXSIT);

	if(service->ret != 0)
		print_error("list", service->ret);
	else
	{
		printf("Multicast IP:  "PRINT_IP_FORMAT"\n", PRINT_NIP(service->multi_ip));
		printf("VM IP List: \n");
		for(i=0; i< service->ip_num; i++){
			printf("    "PRINT_IP_FORMAT"\n", PRINT_NIP(service->ip_list[i]));               
		}
	}
}

int tmcc_service_list(struct tmcc_nl_service_st *serv, uint32_t datalen)
{
    uint32_t msg_len;
    struct tmcc_msg_st *message;
    struct tmcc_nl_show_service_st *service;

    tmcc_comm_kernel_prepare();

    service = (struct tmcc_nl_show_service_st *)malloc(sizeof(struct tmcc_nl_show_service_st));
    if(service == NULL){
		fprintf(stderr, "mem allocate failure!\n");
		close(skfd);
		return -1;
    }

    msg_len = NLMSG_LENGTH(datalen + sizeof(void *));
    message = (struct tmcc_msg_st *)malloc(msg_len);
    if(message == NULL)
    {
	 fprintf(stderr, "mem allocate failure!\n");
     free(service);
	 close(skfd);
	 return -1;
    }

	//printf("multi_ip is %04x\n",serv->multi_ip);

    memset(message, 0, msg_len);	
    message->hdr.nlmsg_flags = NLM_F_REQUEST;
    message->hdr.nlmsg_pid = local.nl_pid;		
    message->hdr.nlmsg_type = TMCC_SERVICE_LIST;
    message->hdr.nlmsg_len = msg_len;
    message->reply_ptr = service;
    memcpy(message->data, serv, datalen);
    
    tmcc_comm_kernel_finish();
	
    service = (struct tmcc_nl_show_service_st *)message->reply_ptr;
    print_vm_ip_info(service);
    free(service);
    free(message);
	
    return reply.err.error;
}

static void print_drop_stats_info(struct multi_server_drop_stats *stats, uint8_t action)
{
    if((action == 1) && (stats->ret != 0)){
        printf("error: multicast is not exist!\n\n");
        return;
    }

    printf("tx packets                     :  %lu\n", stats->tx_packets);
    printf("tx bytes                       :  %lu\n", stats->tx_bytes);

    if(action == 0){
        printf("no_multi_grp_pkt_drop_count    :  %lu\n", stats->no_multi_grp_pkt_drop_count);
        printf("no_multi_member_pkt_drop_count :  %lu\n", stats->no_multi_member_pkt_drop_count);
        printf("route_fail_drop_count          :  %lu\n", stats->route_fail_drop_count);
        printf("no_mem_drop_count              :  %lu\n", stats->no_mem_drop_count);
    }

    printf("\n");

    return;
}

int multi_drop_stats(struct tmcc_nl_service_st *serv, uint32_t datalen)
{
    uint8_t action;
    uint32_t msg_len;
    struct tmcc_msg_st *message;
    struct multi_server_drop_stats stats;

    memset(&stats, 0, sizeof(struct multi_server_drop_stats)); 

    tmcc_comm_kernel_prepare();

    msg_len = NLMSG_LENGTH(datalen + sizeof(void *));
    message = (struct tmcc_msg_st *)malloc(msg_len);
    if(message == NULL)
    {
	 fprintf(stderr, "mem allocate failure!\n");
	 close(skfd);
	 return -1;
    }

	//printf("multi_ip is %04x\n",serv->multi_ip);

    memset(message, 0, msg_len);	
    message->hdr.nlmsg_flags = NLM_F_REQUEST;
    message->hdr.nlmsg_pid = local.nl_pid;		
    message->hdr.nlmsg_type = MULTI_DROP_STATS;
    message->hdr.nlmsg_len = msg_len;
    message->reply_ptr = &stats;
    memcpy(message->data, serv, datalen);
    
    tmcc_comm_kernel_finish();
	
    //service = (struct tmcc_nl_show_service_st *)message->reply_ptr;
    action = (serv->multi_ip == 0 ? 0 : 1);
    print_drop_stats_info(&stats, action);
    free(message);
	
    return reply.err.error;
}

void print_multi_grp_ip(struct tmcc_nl_show_service_st *service)
{
    uint32_t i;
    printf("Multicast Group IP: \n");
    for(i = 0; i < service->ip_num; i++)
    {
        printf(""PRINT_IP_FORMAT"\n", PRINT_NIP(service->ip_list[i]));           
    }
}

int tmcc_service_show(struct tmcc_nl_service_st *serv, uint32_t datalen)
{
    uint32_t msg_len;
    struct tmcc_msg_st *message;
    struct tmcc_nl_show_service_st *service;

	tmcc_comm_kernel_prepare();

    service = (struct tmcc_nl_show_service_st *)malloc(sizeof(struct tmcc_nl_show_service_st));
    if(service == NULL){
		fprintf(stderr, "mem allocate failure!\n");
		close(skfd);
		return -1;
    }

    msg_len = NLMSG_LENGTH(sizeof(void *));
    message = (struct tmcc_msg_st *)malloc(msg_len);
    if(message == NULL){
		fprintf(stderr, "mem allocate failure!\n");
        free(service);
		close(skfd);
		return -1;
	}

	memset(message, 0, msg_len);	
	message->hdr.nlmsg_flags = NLM_F_REQUEST;
	message->hdr.nlmsg_pid = local.nl_pid;		
	message->hdr.nlmsg_type = TMCC_SERVICE_SHOW;
	message->hdr.nlmsg_len = msg_len;
    message->reply_ptr = service;
    
	tmcc_comm_kernel_finish();
	
	service = (struct tmcc_nl_show_service_st *)message->reply_ptr;
    print_multi_grp_ip(service);
    free(message);
    free(service);
	
	return reply.err.error;
}

int multi_server_clear_from_kernel(void)
{
    uint32_t msg_len;
    struct tmcc_msg_st *message;

	tmcc_comm_kernel_prepare();

    msg_len = NLMSG_LENGTH(0);
    message = (struct tmcc_msg_st *)malloc(msg_len);
    if(message == NULL){
		fprintf(stderr, "mem allocate failure!\n");
		close(skfd);
		return -1;
	}

	memset(message, 0, msg_len);	
	message->hdr.nlmsg_flags = NLM_F_REQUEST;
	message->hdr.nlmsg_pid = local.nl_pid;		
	message->hdr.nlmsg_type = MULTI_SERVER_CLEAR;
	message->hdr.nlmsg_len = msg_len;
    
	tmcc_comm_kernel_finish();
	
    free(message);
	
	return reply.err.error;
}

