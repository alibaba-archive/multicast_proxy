#ifndef _CONFIG_H_
#define _CONFIG_H_

//#include "winsock.h"
#include "log.h"
#include "connectInit.h"
extern unsigned int groupIp[256];
extern int groupNum;
extern unsigned int gmax_multi_ip;
extern unsigned int gmin_multi_ip;

void multi_ip_max_min_init();
/*
 * func:  load the configuration and add them to the hash
 * param£ºfilepath the location of file
 * return: success 0; fail -1 
 */
int cfg_init(char filepath[], unsigned int groupIpArr[], int *groupNum);
int json_add_list( char multi_ip[], char ip_list[][32] , int num , int server_port);
int json_del_list(char multi_ip[], char ip_list[][32] , int num);
int json_clear_list();
int json_del_multicast(char multi_ip []);
int json_list_multicast(char multi_ip []);
int reload_json_file();
int get_drop_stats();
void * terminal_command(void * lpdwThreadParam);
//DWORD WINAPI terminal_command(LPVOID lpdwThreadParam);

#endif
