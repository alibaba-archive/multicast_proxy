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
	TMCC_APP_CMD_RECOVER = 0,
};

static const char *prog_name;
static const char *short_opt = "RH";

static const struct option long_opts[] = {
    { "recover"  , 0 , 0 , 'R' } ,
    { "help"  , 0 , 0 , 'H' }
};

static void print_usage()
{
    printf("%s -- This command can be used to multicast server configure.\n", prog_name);
    printf("Usage:\n");
    printf("    multis_config_admin  -R\n");
    printf("    multis_config_admin  -H \n");
    printf("Options:\n");
    printf("    -R/--recover	    recover configure\n");
    printf("    -H/--help           help info\n");
}

int main(int argc, char **argv)
{
    int 	cmd;
    int 	opt;
    int	ret = 0;
	
    prog_name = argv[0];

    if (argc < 2)
    {
        print_usage();
        return 0;								
    }

    while ((opt = getopt_long(argc, argv, short_opt, long_opts, NULL)) != -1)
    {
        switch (opt)
        {
            case 'R':
                cmd = TMCC_APP_CMD_RECOVER;
                break;
			case 'H':
				print_usage();
				return 0;
			default:
				print_usage();
				return 0;
		}
	}

	switch(cmd){
		case TMCC_APP_CMD_RECOVER:
            ret = multi_server_config_recover();
            break;
        default:
            print_usage();
            return 0;
    }

    return ret;
}
