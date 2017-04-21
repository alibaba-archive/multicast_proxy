#ifndef __BASE_CONTROL__
#define __BASE_CONTROL__

#include "tmcc_nl.h"

#define MAX_URL_LEN 1024

#define  PRINT_NIP(x)\
    ((x >>  24) & 0xFF),\
    ((x >>  16) & 0xFF),\
    ((x >> 8) & 0xFF),\
    ((x >> 0) & 0xFF)

#define PRINT_IP_FORMAT         "%u.%u.%u.%u"

#define tmcc_comm_kernel_prepare()	\
	struct sockaddr_nl 	local;	\
	struct sockaddr_nl 	kpeer;	\
	struct tmcc_reply_st 	reply;	\
	socklen_t 		kpeerlen;	\
	int 			rcvlen;		\
					\
	skfd = socket(PF_NETLINK, SOCK_RAW, MULTI_NL);	\
	if(skfd < 0)					\
	{						\
		fprintf(stderr, "can not create a netlink socket\n");	\
		return -1;				\
	}		\
	\
	memset(&local, 0, sizeof(local));		\
	local.nl_family = AF_NETLINK;			\
	local.nl_pid = getpid();			\
	local.nl_groups = 0;				\
		\
	struct timeval timeout = {TMCC_MSG_TIMEOUT,0}; 		\
	setsockopt(skfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));	\
	\
	if(bind(skfd, (struct sockaddr*)&local, sizeof(local)) != 0)	\
	{								\
		fprintf(stderr, "bind() error\n");			\
		return -1;						\
	}					\
	\
	memset(&kpeer, 0, sizeof(kpeer));		\
	kpeer.nl_family = AF_NETLINK;			\
	kpeer.nl_pid = 0;				\
	kpeer.nl_groups = 0;				\
							\

#define tmcc_comm_kernel_finish()		\
	sendto(skfd, message, message->hdr.nlmsg_len, 0, (struct sockaddr*)&kpeer, sizeof(kpeer));	\
	while(1)	\
	{   		\
		kpeerlen = sizeof(struct sockaddr_nl);		\
		rcvlen = recvfrom(skfd, &reply, sizeof(struct tmcc_reply_st), 0, (struct sockaddr*)&kpeer, &kpeerlen);		\
		if(rcvlen < 0){	\
			fprintf(stderr, "receive message from netlink error return %d!\n", rcvlen);	\
			close(skfd);		\
			return -1;		\
		}		\
		break;				\
	}  	\
		\
	close(skfd);

#endif
