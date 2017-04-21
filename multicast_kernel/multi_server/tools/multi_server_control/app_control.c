#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <assert.h>
#include <linux/netlink.h>

//#include "tmc_control_pkt.h"
#include "tmcc_nl.h"
#include "parse_arg.h"
#include "grp.h"
#include "json.h"
#include "multis_json_config.h"

enum {
	TMCC_APP_CMD_ADD = 0,
	TMCC_APP_CMD_DEL,
    TMCC_APP_CMD_CLEAR,
	TMCC_APP_CMD_DROP_STATS,
	TMCC_APP_CMD_SHOW,
	TMCC_APP_CMD_LIST
};

#define TMCC_OPT_MULTI_IP         	0x01
#define TMCC_OPT_MEM_IPLIST         0x02
#define TMCC_OPT_JOIN         	    0x04
#define TMCC_OPT_EXIT               0x08

static const char *prog_name;
static const char *short_opt = "ADCPLSHm:j:q:";

static const struct option long_opts[] = {
    { "add"  , 0 , 0 , 'A' } ,
    { "delete"  , 0 , 0 , 'D' } ,
    { "clear"  , 0 , 0 , 'C' } ,
    { "stats"  , 0 , 0 , 'P' } ,
    { "list"  , 0 , 0 , 'L' } ,
    { "show"  , 0 , 0 , 'S' } ,
    { "help"  , 0 , 0 , 'H' } ,
    { "multiip"  , 1 , 0 , 'm' } ,
    { "join"  , 1 , 0 , 'j' } ,
    { "quit"  , 1 , 0 , 'q' } ,
};

static void print_usage()
{
    printf("%s -- This command can be used to configure multicast server.\n", prog_name);
    printf("Usage:\n");
    printf("    multis_admin  -A -m {multi_ip} -j {ip1,ip2,ip3...}\n");
    printf("    multis_admin  -A -m {multi_ip} -q {ip1,ip2,ip3...}\n");
    printf("    multis_admin  -D -m {multi_ip} \n");
    printf("    multis_admin  -C \n");
    printf("    multis_admin  -P -m {multi_ip}\n");
    printf("    multis_admin  -L -m {multi_ip} \n");
    printf("    multis_admin  -S \n");
    printf("    multis_admin  -H \n");
    printf("Options:\n");
    printf("    -A/--add      add multicast group\n");
    printf("    -D/--delete   del multicast group\n");
    printf("    -C/--clear    clear multicast group\n");
    printf("    -P/--stats    packets statistic\n");
    printf("    -S/--show     show multicast group\n");
    printf("    -L/--list     list multicast group member\n");
    printf("    -H/--help     help info\n");
    printf("    -j/--join     vm join multicast group\n");
    printf("    -q/--quit     vm quit multicast group\n");
    printf("    -m/--multiip  multicast ip\n");
}

void print_error(char *action, int errno)
{
    if(errno == 0)
        return;
    //fprintf(stderr, "%s Failed:", action);
    switch(errno){
        case MULTI_NODE_EXSIT:
            fprintf(stderr, "%s\n", "Multicast group already exsit!");
            break;
        case MULTI_NODE_NOT_EXSIT:
            fprintf(stderr, "%s\n", "Multicast group not exsit,please create first!");
            break;
        case MULTI_IDX_EXHAUST:
            fprintf(stderr, "%s\n", "The index for multicast group exhaust!");
            break;
        case MULTI_PARAM_ERROR:
            fprintf(stderr, "%s\n", "The parameter is error,need check parameter!");
            break;
        case MULTI_VM_LAGER_THAN_MAX:
            printf("the vm number in multicast group shoule smaller than %d!\n", MULTI_VM_MAX);
            break;
        case TMCC_CMD_OPTION_ERROR:
            fprintf(stderr, "%s\n", "option error");
            break;
        default:
            break;
    //        fprintf(stderr, "	%s(%d)\n", "unknown error number!", errno);
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
    //if(ret != 0)
    //    multi_grp_del_from_config(service);

    return ret;
}

int tmcc_nl_service_del(struct tmcc_nl_service_st *service)
{
    int ret = 0;

    if(multi_grp_del_from_config(service) != 0)
        return -1;

    //delete service information from kernel
    ret = tmcc_service_delete_from_kernel(service, sizeof(struct tmcc_nl_service_st));
    //if(ret != 0)
    //    multi_grp_add_to_config(service);

    return ret;
}

int tmcc_nl_service_clear(void)
{
    multi_server_config_clear();
    return multi_server_clear_from_kernel();
}

int tmcc_nl_service_list(struct tmcc_nl_service_st *service)
{
    //list service information from kernel
    return tmcc_service_list(service, sizeof(struct tmcc_nl_service_st));
}

int tmcc_nl_service_show(struct tmcc_nl_service_st *service)
{
    //list service information from kernel
    return tmcc_service_show(service, sizeof(struct tmcc_nl_service_st));
}

struct tmcc_nl_service_st service;
int main(int argc, char **argv)
{
    int 	cmd = -1;
    int 	opt = -1;
    int mask = 0;
    int	ret = 0;
    uint32_t multi_ip;
	
    prog_name = argv[0];

    if (argc < 2)
    {
        print_usage();
        return 0;								
    }

    if(multi_grp_config_init(MULTI_SERVER_CONFIG_FILE) != 0)
        return -1;

    memset(&service, 0, sizeof(struct tmcc_nl_service_st));
    service.action = MULTI_OPT_JOIN;

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
                cmd = TMCC_APP_CMD_DROP_STATS;
                break;
            case 'L':
                cmd = TMCC_APP_CMD_LIST;
                break;
            case 'S':
                cmd = TMCC_APP_CMD_SHOW;
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

                if(!((multi_ip >= 0xE0000000) && (multi_ip <= 0xEFFFFFFF))){
                    printf("error: ip %s is not a multicast ip!\n\n", optarg);
                    return -1;
                }

                service.multi_ip = multi_ip;
                break;
            case 'j':
                if((mask & TMCC_OPT_JOIN) != 0){ 
                    printf("error: option -j repeated\n\n");
                    return -1;
                }
                mask |= TMCC_OPT_JOIN;
                ret = parse_service(optarg, &service);
				if(ret != 0){
					print_error("parse iplist", ret);
					return ret;
				}	
                service.action = MULTI_OPT_JOIN;
                break;
	     case 'q':
                if((mask & TMCC_OPT_EXIT) != 0){ 
                    printf("error: option -q repeated\n\n");
                    return -1;
                }
                mask |= TMCC_OPT_EXIT;
                ret = parse_service(optarg, &service);
				if(ret != 0){
					print_error("parse iplist:", ret);
					return ret;
				}	
                service.action = MULTI_OPT_QUIT;
                break;
            default:
                print_usage();
                return -1;
        }
    }

    switch(cmd){
        case TMCC_APP_CMD_ADD:
            if((mask != (TMCC_OPT_MULTI_IP | TMCC_OPT_JOIN)) &&
               (mask != (TMCC_OPT_MULTI_IP | TMCC_OPT_EXIT)) &&
               (mask != TMCC_OPT_MULTI_IP)){
                printf("add option error\n\n");
                return -1;
            }  
            ret = tmcc_nl_service_add(&service);
            print_error("add", ret);	
            break;
        case TMCC_APP_CMD_DEL:
            if(mask != TMCC_OPT_MULTI_IP){
                printf("del option error\n\n");
                return -1;
            }  
            ret = tmcc_nl_service_del(&service);
           	print_error("del", ret);	
            break;
        case TMCC_APP_CMD_CLEAR:
            if(mask != 0){
                printf("error: no need option\n\n");
                return -1;
            }  

            ret = tmcc_nl_service_clear();
           	print_error("clear", ret);	
            break;
        case TMCC_APP_CMD_DROP_STATS:
            if((mask != 0) && (mask != TMCC_OPT_MULTI_IP)){
                printf("error: option error!\n\n");
                return -1;
            }
            ret = multi_drop_stats(&service, sizeof(struct tmcc_nl_service_st));
           	print_error("stats", ret);	
            break;
        case TMCC_APP_CMD_LIST:
            if(mask != TMCC_OPT_MULTI_IP){
                printf("list option error\n\n");
                return -1;
            }  
            ret = tmcc_nl_service_list(&service);
            break;
        case TMCC_APP_CMD_SHOW:
            if(mask != 0){
                printf("error: no option needed!\n\n");
                return -1;
            }
            ret = tmcc_nl_service_show(&service);
            print_error("show", ret);	
            break;
        default:
            print_usage();
            return -1;
    }

    return ret;
}
