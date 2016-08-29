#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <getopt.h>
#include "connectInit.h"
#include "config.h"
#include "handleData.h"
#include "worker_thread.h"
#include "parse_arg.h"
#include "macro_define.h"
#include "entry.h"
enum {
        MULTICAST_APP_CMD_ADD = 0,
        MULTICAST_APP_CMD_DEL,
        MULTICAST_APP_CMD_CLEAR,
        MULTICAST_APP_CMD_STATS,
        MULTICAST_APP_CMD_SHOW,
        MULTICAST_APP_CMD_LIST,
        MULTICAST_APP_CMD_RUN
};

#define MULTICAST_OPT_MULTI_IP 0x01
#define MULTICAST_OPT_SERVER_IP 0x02
#define MULTICAST_OPT_CLIENT_GPORT 0x04
#define MULTICAST_OPT_CLIENT_SPORT 0x08

static const char * prog_name;
static const char * short_opt = "ADCPLSRHm:i:g:s:";

static const struct option long_opts[] = {
    { "add"  , 0 , 0 , 'A' } ,
    { "delete"  , 0 , 0 , 'D' } ,
    { "clear"  , 0 , 0 , 'C' } ,
    { "stats"  , 0 , 0 , 'P' } ,
    { "list"  , 0 , 0 , 'L' } ,
    { "run"   , 0 , 0 , 'R' },
    { "help"  , 0 , 0 , 'H' } ,
    { "multiip"  , 1 , 0 , 'm' } ,
    { "ip"  , 1 , 0 , 'i' } ,
    { "gport"  , 1 , 0 , 'g' } ,
    { "sport"  , 1 , 0 , 's' }
};
static void print_usage()
{
    printf("%s -- This command can be used to configure multicast client.\n", prog_name);
    printf("Usage:\n");
    printf("    multic_admin  -A -i {ip} -g {gourpport} -s {serverport} -m {multi_ip}\n");
    printf("    multic_admin  -D -i {ip} -g {groupport} \n");
    printf("    multic_admin  -C \n");
    printf("    multic_admin  -P -i {ip} -g {groupport} \n");
    printf("    multic_admin  -L\n");
    printf("    multic_admin  -H \n");
    printf("Options:\n");
    printf("    -A/--add                add multicast server ip and port\n");
    printf("    -D/--delete             del multicast server ip and port\n");
    printf("    -C/--clear              clear multicast server information\n");
    printf("    -P/--stats              recv packets statistic\n");
    printf("    -L/--list               list all multicast server ip and port\n");
    printf("    -H/--help               help info\n");
    printf("    -i/--ip                 multicast server ip, the ip of multicast provider\n");
    printf("    -g/--gport              udp port, the multicast port\n");
    printf("    -s/--sport              server port\n");
    printf("    -m/--multi_ip           multicast ip\n");
}
int main(int argc, char **argv)
{
    int  cmd = -1;
    int  opt = -1;
    uint32_t mask = 0;
    int ret = 0;
    uint32_t multi_ip, server_ip;
    char multi_ips[32];
    char server_ips[32];
    uint32_t gport;
    uint32_t sport;

    prog_name = argv[0];

    if (argc < 2)
    {
        print_usage();
        return -1;
    }

    while ((opt = getopt_long(argc, argv, short_opt, long_opts, NULL)) != -1)
    {
        switch (opt)
        {
            case 'A':
                cmd = MULTICAST_APP_CMD_ADD;
                break;
            case 'D':
                cmd = MULTICAST_APP_CMD_DEL;
                break;
            case 'C':
                cmd = MULTICAST_APP_CMD_CLEAR;
                break;
            case 'P':
                cmd = MULTICAST_APP_CMD_STATS;
                break;
            case 'L':
                cmd = MULTICAST_APP_CMD_LIST;
                break;
            case 'R':
                cmd = MULTICAST_APP_CMD_RUN;
                break;
            case 'H':
                print_usage();
                return 0;
            case 'm':
                if((mask & MULTICAST_OPT_MULTI_IP) != 0)
                {
                    printf("error: option -m repeated\n\n");
                    return -1;
                }
                mask |= MULTICAST_OPT_MULTI_IP;
                strcpy(multi_ips , optarg);
                ret = parse_ip_be(optarg,&multi_ip);
                if(ret != 0)
                {
                        printf("error: ip %s is invalid!\n\n", optarg);
                        return -1;
                }
                 if(!((multi_ip >= 0xE0000000) && (multi_ip <= 0xEFFFFFFF)))
                 {
                        printf("error: ip %s is not a multicast ip!\n\n", optarg);
                        return -1;
                 }
                 break;
              case 'i':
                 if((mask & MULTICAST_OPT_SERVER_IP) != 0)
                 {
                         printf("error: option -i repeated\n\n");
                         return -1;
                 }
                 mask |= MULTICAST_OPT_SERVER_IP;
                 strcpy(server_ips , optarg );
                 ret = parse_ip_be(optarg, &server_ip);
                 if(ret != 0)
                 {
                          printf("error: ip %s is invalid!\n\n", optarg);
                          return -1;
                 }
                 break;
               case 'g':
                 if((mask & MULTICAST_OPT_CLIENT_GPORT) != 0)
                 {
                          printf("error: option -g repeated.\nTry \'%s --help\' for more information.\n\n", prog_name);
                          return -1;
                 }
                 mask |= MULTICAST_OPT_CLIENT_GPORT;
                 if(parse_u32(optarg, &gport) != 0)
                 {
                          printf("error: the value of option -g is not positive integer!\n\n");
                          return -1;
                 }

                if(gport > 65535)
                 {
                         printf("error: the value of option -g should be smaller than 65536!\n\n");
                         return -1;
                 }
                 break;
	       case 's':
		 if(( mask & MULTICAST_OPT_CLIENT_SPORT) != 0 )
		 {
		    printf("error: option -s repeated. \n Try \'%s --help\' for more information\n\n", prog_name);
                    return -1;
                 }
		 mask |= MULTICAST_OPT_CLIENT_SPORT;
		 if(parse_u32(optarg , &sport) != 0)
		 {
		    	   printf("error: the value of option -s is not positive integer!\n\n");
			   return -1;
		 }
		 if(sport > 65535)
		 {
		           printf("error: the value of option -s should be smaller than 65536!\n\n ");
			   return -1;
		 }
		 break;
               default:
                  print_usage();
                  return -1;
                }
         }
         switch(cmd)
         {
             case MULTICAST_APP_CMD_ADD:
                   if(mask != (MULTICAST_OPT_MULTI_IP|MULTICAST_OPT_SERVER_IP|MULTICAST_OPT_CLIENT_GPORT|MULTICAST_OPT_CLIENT_SPORT))
                   {
                         printf("option error.\nTry \'%s --help\' for more information.\n\n", prog_name);
                         return -1;
                   }
                   ret = json_add_grp_node(server_ips , gport , sport , multi_ips); 
                   reload_json_file();
                   break;
            case MULTICAST_APP_CMD_DEL:
                   if(mask != (MULTICAST_OPT_SERVER_IP|MULTICAST_OPT_CLIENT_GPORT))
                   {
                        printf("option error.\nTry \'%s --help\' for more information.\n\n", prog_name);
                        return -1;
                   }
                  ret = json_del_grp_node(server_ips, gport);
                  reload_json_file();
                  break;
           case MULTICAST_APP_CMD_CLEAR:
                  if(mask != 0)
                  {
                        printf("option error.\nTry \'%s --help\' for more information.\n\n", prog_name);
                        return -1;
                  }
                   ret = json_clear_grp_node();
                   reload_json_file();
                   break;
           case MULTICAST_APP_CMD_STATS:
                   if((mask != 0) && (mask != (MULTICAST_OPT_SERVER_IP|MULTICAST_OPT_CLIENT_GPORT)))
                   {
                         printf("option error.\nTry \'%s --help\' for more information.\n\n", prog_name);
                         return -1;
                   }
                   ret = get_drop_stats(); 
                   break;
           case MULTICAST_APP_CMD_LIST:
                   if(mask != 0)
                   {
                         printf("option error.\nTry \'%s --help\' for more information.\n\n", prog_name);
                         return -1;
                   }
                   ret = json_list_multicast();
                   break;
           case MULTICAST_APP_CMD_RUN:
                  //printf("multicast client going to run\n");
                  entry();
                  break;
                  printf("clear\n");
           default:
                  print_usage();
                  return -1;
    }

    return ret;
}
