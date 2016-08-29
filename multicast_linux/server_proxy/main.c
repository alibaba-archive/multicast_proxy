#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include "parse_arg.h"
#include "iphdr.h"
#include "config.h"
#include "multi_grp.h"
#include "worker_thread.h"
#include "handleIncomeData.h"
#include "connectInit.h"
#include "macro_define.h"
#include "entry.h"
#include "log.h"

enum {
	MULTICAST_APP_CMD_ADD = 0,
	MULTICAST_APP_CMD_DEL,
	MULTICAST_APP_CMD_CLEAR,
	MULTICAST_APP_CMD_DROP_STATS,
	MULTICAST_APP_CMD_SHOW,
	MULTICAST_APP_CMD_LIST,
	MULTICAST_APP_CMD_RUN
};

#define MULTICAST_OPT_MULTI_IP 0x01
#define MULTICAST_OPT_IPLIST 0x02
#define MULTICAST_OPT_JOIN 0x04
#define MULTICAST_OPT_EXIT 0x08
#define MULTICAST_OPT_PORT 0x10

static const char * prog_name;
static const char * short_opt = "ADCPLSRHm:j:q:p:";

static const struct option long_opts[] = {
	{ "add" , 0 , 0 , 'A' },
	{ "delete" , 0 , 0, 'D'},
        { "clear" , 0 , 0 , 'C'},
	{ "stats" , 0 , 0 , 'P'},
	{ "list"  , 0 , 0 , 'L' },
        { "show"  , 0 , 0 , 'S' },
        { "help"  , 0 , 0 , 'H' },
	{ "run" , 0 , 0 , 'R'},
        { "multiip"  , 1 , 0 , 'm' },
        { "join"  , 1 , 0 , 'j' },
        { "quit"  , 1 , 0 , 'q' },
        { "port"  , 1 , 0 , 'p' } 
};

static void print_usage()
{
    printf("%s -- This command can be used to configure multicast server.\n", prog_name);
    printf("Usage:\n");
    printf("    multis_admin  -A -m {multi_ip} -j {ip1,ip2,ip3...} -p port\n");
    printf("    multis_admin  -A -m {multi_ip} -q {ip1,ip2,ip3...}\n");
    printf("    multis_admin  -D -m {multi_ip} \n");
    printf("    multis_admin  -C \n");
    printf("    multis_admin  -P -m {multi_ip}\n");
    printf("    multis_admin  -L -m {multi_ip} \n");
    printf("    multis_admin  -S \n");
    printf("    multis_admin  -H \n");
    printf("    multis_admin  -R\n");
    printf("Options:\n");
    printf("    -A/--add      add multicast group\n");
    printf("    -D/--delete   del multicast group\n");
    printf("    -C/--clear    clear multicast group\n");
    printf("    -P/--stats    packets statistic\n");
    printf("    -S/--show     show multicast group\n");
    printf("    -L/--list     list multicast group member\n");
    printf("    -R/--run      run the multicast server to transmit packages\n");
    printf("    -H/--help     help info\n");
    printf("    -j/--join     vm join multicast group\n");
    printf("    -q/--quit     vm quit multicast group\n");
    printf("    -m/--multiip  multicast ip\n");
    printf("    -p/--port     server port\n");
}

void signal_handler(int sig)
{
    fprintf(gfp_log, "[%s:%d]signal: %u!\n", __FILE__, __LINE__, sig);
}

int main(int argc , char ** argv)
{
    int cmd = -1;
    int opt = -1;
    int mask = 0;
    int ret = 0;
    int i = 0;
    int join_or_quit = 0;
    uint32_t multi_ip;
    char multi_ips[32];
    char ip_list[MAX_MEM_IP_IN_GROUP][32];
    int ip_num = 0; 
    uint32_t server_port = 7126;
    pthread_t entry_t;
    prog_name = argv[0];
    if (argc < 2)
    {
        print_usage();
	return 0;
    }    

    signal(SIGINT, signal_handler);
    signal(SIGSTOP, signal_handler);
    signal(SIGHUP, signal_handler);

    while(( opt = getopt_long(argc , argv , short_opt , long_opts , NULL)) != -1 )
    {
        switch(opt)
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
               cmd = MULTICAST_APP_CMD_DROP_STATS;
	       break;
	    case 'L':
	       cmd = MULTICAST_APP_CMD_LIST;
	       break;
	    case 'S':
	       cmd = MULTICAST_APP_CMD_SHOW;
	       break;
	    case 'R':
	       cmd = MULTICAST_APP_CMD_RUN;
	       break;
	    case 'H':
	       print_usage();
	       return 0;
	    case 'm':
	       if ( (mask & MULTICAST_OPT_MULTI_IP) != 0 )
	       {
	           printf("error: option -m repeated\n");
		   return -1;
	       }
	       mask |= MULTICAST_OPT_MULTI_IP;
               strcpy(multi_ips , optarg ); 
               ret = parse_ip_be(optarg,&multi_ip);
	       if(ret!=0)
	       {
	           printf("error: ip %s is invalid!\n",optarg);
		   return -1;
	       }
	       if(!((multi_ip >= 0xE0000000) && (multi_ip <= 0xEFFFFFFF)))
	       {
		   printf("error: ip %s is not a multicast ip!\n\n", optarg);
		   return -1;
	       }
	       break;
	     case 'j':
	       if ( (mask&MULTICAST_OPT_JOIN) != 0 )
	       {
	           printf("error: option -j repeated\n");
		   return -1;
	       }
               join_or_quit = 1;
	       mask |= MULTICAST_OPT_JOIN;
               ip_num = multi_range_num(optarg);
               char optargs[2048];
               memcpy(optargs , optarg , strlen(optarg)+1);
	       char * p_begin = optargs;
	       char * p_end = optargs;
	       int count = 0;
	       while( *p_end != '\0')
	       {
	            p_end ++;
		    count ++;
		    if(*p_end == ',')
		    {
		        strncpy(ip_list[i] , p_begin , count);
			p_end++ ;
			p_begin = p_end;
			i++;
                        count = 0 ;
		    }
		    if(*p_end == '\0')
		    {
		       strncpy(ip_list[i] , p_begin , count);
		    }
	       }
               for(i = 0 ;i <=ip_num ;i++)
	       {
	           printf("%s\n",ip_list[i]);
	       } 
	       /*
		   interface need to done
		*/
	       break; 
	     case 'q':
	       if( (mask&MULTICAST_OPT_EXIT) != 0 )
	       {
		   return -1;
	       }
	       join_or_quit = 0;   
	       mask |= MULTICAST_OPT_EXIT;
               ip_num = multi_range_num(optarg);
               memcpy(optargs , optarg , strlen(optarg)+1);
               p_begin = optargs;
               p_end = optargs;
               count = 0;
               while( *p_end != '\0')
               {
                    p_end ++;
                    count ++;
                    if(*p_end == ',')
                    {
                        strncpy(ip_list[i] , p_begin , count);
                        p_end++ ;
                        p_begin = p_end;
                        i++;
                        count = 0 ;
                    }
                    if(*p_end == '\0')
                    {
                       strncpy(ip_list[i] , p_begin , count);
                    }
               }
	       /*
		    interface need to be done
		
		*/
	       break;
	     case 'p':
	       if((mask & MULTICAST_OPT_PORT) != 0)
	       {
	            printf("error: option -p repeated.\n");
		    return -1;
	       }
	       mask |= MULTICAST_OPT_PORT;
	       if(parse_u32(optarg , &server_port) != 0)
	       {
	             printf("error: the value of option -p is not positive interger!\n\n");
		     return -1;
	       }
	       if(server_port > 65535)
	       {
	             printf("error: the value of option -p should be smaller than 65536!\n\n");
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
	     if ( (mask != (MULTICAST_OPT_MULTI_IP | MULTICAST_OPT_JOIN)) &&\
	          (mask != (MULTICAST_OPT_MULTI_IP | MULTICAST_OPT_EXIT)) &&\
                  (mask != (MULTICAST_OPT_MULTI_IP | MULTICAST_OPT_JOIN | MULTICAST_OPT_PORT))&&\
		  (mask != MULTICAST_OPT_MULTI_IP) )
	     {
		 return -1;
	     }
             if ( join_or_quit )
             {
	         ret = json_add_list(multi_ips, ip_list , ip_num , server_port);
	     }else{
	         ret = json_del_list(multi_ips , ip_list , ip_num);
	     }
             reload_json_file();
           break;
	 case MULTICAST_APP_CMD_DEL:
	     if ( mask != MULTICAST_OPT_MULTI_IP )
	     {
		 return -1;
	     }
             ret = json_del_multicast(multi_ips);   
	     reload_json_file();
             break;
	  case MULTICAST_APP_CMD_CLEAR:
	     if( mask != 0)
	     {
		 return -1;
	     }
             ret = json_clear_list();
             reload_json_file();
	     break;
	  case MULTICAST_APP_CMD_DROP_STATS:
             if ( (mask != 0) && (mask != MULTICAST_OPT_MULTI_IP) )
	     {
		 return -1;
	     }
	     ret = get_drop_stats();
	     break;
	  case MULTICAST_APP_CMD_LIST:
	     if ( mask != MULTICAST_OPT_MULTI_IP )
	     {
		  return -1;
	     }
	     ret = json_list_multicast(multi_ips);
	     break;
	  case MULTICAST_APP_CMD_SHOW:
	     if ( mask != 0)
	     {
		 return -1;
	     }
	     ret = json_show_multicast();
	     break;
          case MULTICAST_APP_CMD_RUN:
	     entry();
             break;
          default:
             print_usage();
	     return -1;
    }
    return ret;
}



















