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

void print_packets_stats(struct multic_packets_stats_st *stats)
{
    if(stats->ret != 0){
        printf("multicast server not existed!\n\n");
        return;
    }

    printf("rx packets  :  %lu\n", stats->rx_packets);
    printf("rx bytes    :  %lu\n\n", stats->rx_bytes);
}

int multi_client_packets_stats(struct tmcc_nl_service_st *serv, uint32_t datalen)
{
	uint32_t msg_len;
	struct tmcc_msg_st *message;
    struct multic_packets_stats_st stats;

    memset(&stats, 0, sizeof(struct multic_packets_stats_st));

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
    message->hdr.nlmsg_type = MULTIC_PACKETS_STATS;
    message->hdr.nlmsg_len = msg_len;
    message->reply_ptr = &stats;
    memcpy(message->data, serv, datalen);
    
    tmcc_comm_kernel_finish();
	
    print_packets_stats(&stats);
    free(message);
	
    return reply.err.error;
}

void print_vm_ip_info(struct tmcc_nl_show_service_st *service)
{
	uint32_t i;

	//printf("service-ret %u node_cnt %u\n",service->ret, service->node_cnt);

	if(service->ret != 0)
		printf("list group info error ret = %d\n", service->ret);
	else
	{
		if(service->node_cnt == 0)
			printf("Not configure multicast server ip and port!\n");
		for(i = 0; i < service->node_cnt; i++)
		{
			printf("Configure Multicast Server %d:\n",i+1);
			printf("Server IP:  "PRINT_IP_FORMAT"\n", PRINT_NIP(service->server_ip[i]));
			printf("UDP Port:  %u\n", service->port[i]);
			printf("Multicast  IP:  "PRINT_IP_FORMAT"\n", PRINT_NIP(service->multi_ip[i]));
			printf("\n\n");
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

int multi_client_clear(void)
{
	uint32_t msg_len;
	struct tmcc_msg_st *message;

    tmcc_comm_kernel_prepare();

    msg_len = NLMSG_LENGTH(0);
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
    message->hdr.nlmsg_type = MULTI_CLIENT_CLEAR;
    message->hdr.nlmsg_len = msg_len;
    
    tmcc_comm_kernel_finish();
	
    free(message);
	
    return reply.err.error;
}
