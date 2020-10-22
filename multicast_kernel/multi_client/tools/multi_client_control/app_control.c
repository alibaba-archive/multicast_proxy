#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>                                                                          
#include <sys/time.h>
#include <arpa/inet.h>
#include <linux/netlink.h>

#include "tmcc_nl.h"
#include "parse_arg.h"
#include "grp.h"
#include "json.h"
#include "multic_json_config.h"

enum {
	TMCC_APP_CMD_ADD = 0,
	TMCC_APP_CMD_DEL,
	TMCC_APP_CMD_CLEAR,
	TMCC_APP_CMD_STATS,
	TMCC_APP_CMD_SHOW,
	TMCC_APP_CMD_LIST
};

#define TMCC_OPT_MULTI_IP         	0x01
#define TMCC_OPT_SERVER_IP         	0x02
#define TMCC_OPT_CLIENT_PORT        0x04

static const char *prog_name;
static const char *short_opt = "ADPCLSHm:i:p:";

static const struct option long_opts[] = {
    { "add"  , 0 , 0 , 'A' } ,
    { "delete"  , 0 , 0 , 'D' } ,
    { "clear"  , 0 , 0 , 'C' } ,
    { "stats"  , 0 , 0 , 'P' } ,
    { "list"  , 0 , 0 , 'L' } ,
    { "help"  , 0 , 0 , 'H' } ,
    { "multiip"  , 1 , 0 , 'm' } ,
    { "ip"  , 1 , 0 , 'i' } ,
    { "port"  , 1 , 0 , 'p' } ,
};

static void print_usage()
{
    printf("%s -- This command can be used to configure multicast client.\n", prog_name);
    printf("Usage:\n");
    printf("    multic_admin  -A -i {ip} -p {port} -m {multi_ip}\n");
    printf("    multic_admin  -D -i {ip} -p {port} \n");
    printf("    multic_admin  -C \n");
    printf("    multic_admin  -P -i {ip} -p {port} \n");
    printf("    multic_admin  -L\n");
    printf("    multic_admin  -H \n");
    printf("Options:\n");
    printf("    -A/--add		add multicast server ip and port\n");
    printf("    -D/--delete		del multicast server ip and port\n");
    printf("    -C/--clear		clear multicast server information\n");
    printf("    -P/--stats		recv packets statistic\n");
    printf("    -L/--list		list all multicast server ip and port\n");
    printf("    -H/--help           help info\n");
    printf("    -i/--ip             multicast server ip, the ip of multicast provider\n");
    printf("    -p/--port           udp port, the multicast port\n");
    printf("    -m/--multi_ip       multicast ip\n");
}

void print_error(char *action, int errno)
{
    if(errno == 0)
        return;
    switch(errno){
        case RPINT_ERROR:
            printf("error: %s\n", action);
            break;
        case MULTI_NODE_EXSIT:
            fprintf(stderr, "%s\n\n", "Multicast group already exsit!");
            break;
        case MULTI_NODE_NOT_EXSIT:
            fprintf(stderr, "%s\n\n", "Multicast group not exsit,please create first!");
            break;
        case MULTI_IDX_EXHAUST:
            fprintf(stderr, "%s\n\n", "The index for multicast group exhaust!");
            break;
        case MULTI_PARAM_ERROR:
            fprintf(stderr, "%s\n\n", "The parameter is error, need check parameter!");
            break;
        case MULTI_TBL_FULL:
            fprintf(stderr, "You can only add %d multicast group!\n", MULTI_GRP_MAX);
            break;
        case TMCC_CMD_OPTION_ERROR:
            fprintf(stderr, "%s\n\n", "option error");
            break;
        default:
            break;
        //    fprintf(stderr, "	%s(%d)\n\n", "unknown error number!", errno);
    }
    printf("Try \'%s --help\' for more information.\n\n", prog_name);
}

int tmcc_nl_service_add(struct tmcc_nl_service_st *service)
{
    int ret = 0;

    if(multi_grp_add_to_config(service) != 0)
        return -1;

    //add multi group information to kernel
    ret = tmcc_service_add_to_kernel(service, sizeof(struct tmcc_nl_service_st));
    if(ret != 0)
        multi_grp_del_from_config(service);

    return ret;

}

int tmcc_nl_service_del(struct tmcc_nl_service_st *service)
{
    int ret = 0;

    if(multi_grp_del_from_config(service) != 0)
        return -1;

    //delete service information from kernel
    ret = tmcc_service_delete_from_kernel(service, sizeof(struct tmcc_nl_service_st));
    if(ret != 0)
        multi_grp_add_to_config(service);

    return ret;
}


int tmcc_nl_service_list(struct tmcc_nl_service_st *service)
{
    //list service information from kernel
    return tmcc_service_list(service, sizeof(struct tmcc_nl_service_st));
}

int multi_nl_client_clear(void)
{
    multi_client_clear_config();
    return multi_client_clear();
}


struct tmcc_nl_service_st service;
int main(int argc, char **argv)
{
    int 	cmd = -1;
    int 	opt = -1;
    uint32_t mask = 0;
    int	ret = 0;
    uint32_t multi_ip, server_ip;
	uint32_t port;
	
    prog_name = argv[0];

    if (argc < 2)
    {
        print_usage();
        return -1;								
    }

    if(multi_grp_config_init(MULTI_CLIENT_CONFIG_FILE) != 0)
        return -1;

    memset(&service, 0, sizeof(struct tmcc_nl_service_st));

    while ((opt = getopt_long(argc, argv, short_opt, long_opts, NULL)) != -1)
    {
        switch (opt)
        {
            case 'A':
                cmd = TMCC_APP_CMD_ADD;
                break;
            case 'D':
				cmd = TMCC_APP_CMD_DEL;
				break;
            case 'C':
				cmd = TMCC_APP_CMD_CLEAR;
				break;
            case 'P':
				cmd = TMCC_APP_CMD_STATS;
				break;
			case 'L':
				cmd = TMCC_APP_CMD_LIST;
				break;
			case 'H':
				print_usage();
				return 0;
			case 'm':
                if((mask & TMCC_OPT_MULTI_IP) != 0){
                    printf("error: option -m repeated\n\n");
                    return -1;
                }
                mask |= TMCC_OPT_MULTI_IP;
				ret = parse_ip_be(optarg,&multi_ip);
				if(ret != 0){
					printf("error: ip %s is invalid!\n\n", optarg);
					return -1;
				}
#if 0
                if(!((multi_ip >= 0xE0000000) && (multi_ip <= 0xEFFFFFFF))){
                    printf("error: ip %s is not a multicast ip!\n\n", optarg);
                    return -1;
                }
#endif
				service.multi_ip = multi_ip;
				break;
			case 'i':
                if((mask & TMCC_OPT_SERVER_IP) != 0){
                    printf("error: option -i repeated\n\n");
                    return -1;
                }
                mask |= TMCC_OPT_SERVER_IP;
				ret = parse_ip_be(optarg, &server_ip);
				if(ret != 0)
				{
					printf("error: ip %s is invalid!\n\n", optarg);
					return -1;
				}	
				service.server_ip = server_ip;
				break;
			case 'p':
                if((mask & TMCC_OPT_CLIENT_PORT) != 0){
                    printf("error: option -p repeated.\nTry \'%s --help\' for more information.\n\n", prog_name);
                    return -1;
                }
                mask |= TMCC_OPT_CLIENT_PORT;
				if(parse_u32(optarg, &port) != 0){
                    printf("error: the value of option -p is not positive integer!\n\n");
                    return -1;
                }

                if(port > 65535){
                    printf("error: the value of option -p should be smaller than 65536!\n\n");
                    return -1;
                }
 
				service.port = port;
				break;
			default:
				print_usage();
				return -1;
		}
	}

	switch(cmd){
		case TMCC_APP_CMD_ADD:
            if(mask != (TMCC_OPT_MULTI_IP|TMCC_OPT_SERVER_IP|TMCC_OPT_CLIENT_PORT)){
                printf("option error.\nTry \'%s --help\' for more information.\n\n", prog_name);
                return -1;
            }
            ret = tmcc_nl_service_add(&service);
            print_error("add", ret);	
            break;
        case TMCC_APP_CMD_DEL:
            if(mask != (TMCC_OPT_SERVER_IP|TMCC_OPT_CLIENT_PORT)){
                printf("option error.\nTry \'%s --help\' for more information.\n\n", prog_name);
                return -1;
            }
            ret = tmcc_nl_service_del(&service);
           	print_error("del", ret);	
            break;
        case TMCC_APP_CMD_CLEAR:
            if(mask != 0){
                printf("option error.\nTry \'%s --help\' for more information.\n\n", prog_name);
                return -1;
            }
            ret = multi_nl_client_clear();
           	print_error("clear", ret);	
            break;
        case TMCC_APP_CMD_STATS:
            if((mask != 0) && (mask != (TMCC_OPT_SERVER_IP|TMCC_OPT_CLIENT_PORT))){
                printf("option error.\nTry \'%s --help\' for more information.\n\n", prog_name);
                return -1;
            }
            ret = multi_client_packets_stats(&service, sizeof(struct tmcc_nl_service_st));
            break;
        case TMCC_APP_CMD_LIST:
            if(mask != 0){
                printf("option error.\nTry \'%s --help\' for more information.\n\n", prog_name);
                return -1;
            }
            ret = tmcc_nl_service_list(&service);
            break;
        default:
            print_usage();
            return -1;
    }

    return ret;
}
