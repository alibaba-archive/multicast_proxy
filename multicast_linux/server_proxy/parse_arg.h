#ifndef _TMC_CMD_COMMON_H_
#define _TMC_CMD_COMMON_H_
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
static inline int parse_ip_be(char *arg, uint32_t *ip)
{
	int ret;
	struct in_addr addr;

	if(arg == NULL || ip == NULL) {
		return -1;
	}

    ret = inet_pton(AF_INET, arg, (unsigned char* )&addr);
	if(ret <= 0) {
		return -1;
	}

	*ip = htonl(addr.s_addr);
	return 0;
}

static inline int multi_range_num(char *arg)
{
	int ret = 0;
	char *p;

	if(arg == NULL) {
		ret = 0;
		goto out;
	}

	p = arg;
	while(*p != '\0') {
		if(*p == ',') {
			ret ++;
		}
		p ++;
	}

	ret ++;
out:
	return ret;
}



static inline int isalldigit(char *arg)
{
    unsigned long i, len;

    len = strlen(arg);
    for(i=0; i<len; i++){
        if((arg[i] < '0') || (arg[i] > '9'))
            return -1;
    }

    return 0;
}

static inline int parse_u32(char *arg, uint32_t *num)
{
    long long ret;

    if(!arg || !num) {
        return -1;
    }   

    if(isalldigit(arg) == -1)
        return -1;

    ret = strtoll(arg, NULL, 0); 
    if(ret < 0 || ret > 0xffffffff) {
        return -1;
    }   

    *num = ret;
    return 0;
}

#endif

