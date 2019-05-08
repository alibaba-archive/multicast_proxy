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
	if(ret <= 0) {
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
int check_repeated(uint32_t ip, struct tmcc_nl_service_st *service)
{
    uint16_t i;
    for(i = 0; i < service->ip_num; i++){
        if(ip == service->ip_list[i])
            return 1;
    }
    return 0;
}
static inline int parse_service(char *ipstr, struct tmcc_nl_service_st *service)
{
	int ret;
	int num;
    uint32_t ip;
	char *p1;
	char *p2;
	char buf[128];
	int i;
	if(ipstr == NULL || service == NULL) {
		ret = MULTI_PARAM_ERROR;
		goto out;
	}
	num = multi_range_num(ipstr);
	if(num < 0){
		ret = MULTI_PARAM_ERROR;
		goto out;
	}
    if(num > MULTI_VM_MAX){
        ret = MULTI_VM_LAGER_THAN_MAX;
        goto out;
    }
	service->ip_num = 0;
	p1 = ipstr;
	p2 = ipstr;
	for(i = 0; i < num; i ++) {
		while(*p2 != '\0' && *p2 != ',') {
			p2 ++;
		}
		if(p2 - p1 >= sizeof(buf)) {
			ret = MULTI_PARAM_ERROR;
			goto out;
		}
		memset(buf, 0, sizeof(buf));
		memcpy(buf, p1, p2 - p1);
        ret = parse_ip_be(buf, &ip);
		if(ret != 0) {
			ret = MULTI_PARAM_ERROR;
			goto out;
		}
        if((ip >= 0xE0000000) && (ip <= 0xEFFFFFFF)){
            printf("warning: multicast member ip %s is a multicast ip! Just pass.\n\n", buf);
            goto CONTINUE; 
        }   
        if(check_repeated(ip, service) == 0){
            service->ip_list[service->ip_num] = ip;
	        service->ip_num++;
        }
CONTINUE:
		p2 ++;
		p1 = p2;
	}
	ret = 0;
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
