#ifndef _TMC_CMD_COMMON_H_
#define _TMC_CMD_COMMON_H_

static inline int parse_ip_be(char *arg, uint32_t *ip)
{
	int ret;
	struct in_addr addr;

	if(arg == NULL || ip == NULL) {
		return MULTI_PARAM_ERROR;
	}

    ret = inet_pton(AF_INET, arg, (unsigned char* )&addr);
	if(ret == 0) {
		return MULTI_PARAM_ERROR;
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
        return -TMCC_CMD_PARAMS_NULL;
    }   

    if(isalldigit(arg) == -1)
        return -TMCC_CMD_U32_TRANS_ERROR;

    ret = strtoll(arg, NULL, 0); 
    if(ret < 0 || ret > 0xffffffff) {
        return -TMCC_CMD_U32_TRANS_ERROR;
    }   

    *num = ret;
    return 0;
}

#endif

