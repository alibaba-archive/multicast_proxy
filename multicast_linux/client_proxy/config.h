#ifndef _CONFIG_H_
#define _CONFIG_H_
#include "log.h" 
#include <stdint.h>
/*
 * func:  load the configuration and add them to the hash
 * param£ºfilepath: the location of file
 * return: success 0; fail -1 
 */
int cfg_init(char *filepath);
void * cfg_reload(void * lpdwThreadParam);
int json_add_grp_node(char serverip[] , uint32_t groupport , uint32_t serverport ,  char multi_ip[]);
int json_del_grp_node(char serverip[] , uint32_t groupport);
int json_clear_grp_node();
int reload_json_file();
int get_drop_stats();
int json_list_multicast();
#endif
