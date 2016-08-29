#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "winsock.h"
#include "log.h"
#define HAVE_REMOTE
#include "pcap.h"

#define MAX_GROUP 512

extern unsigned int groupIp[MAX_GROUP];
extern int groupNum;
extern unsigned int gmax_multi_ip;
extern unsigned int gmin_multi_ip;

#define FILTER_STRING_SIZE 1024
extern char gbpf_filter_string[FILTER_STRING_SIZE];
extern char errbuf[PCAP_ERRBUF_SIZE];
extern long finish_flag; 
extern int count_dev;
extern pcap_if_t *alldevs;

void multi_ip_max_min_init();
/*
 * func:  load the configuration and add them to the hash
 * param£ºfilepath the location of file
 * return: success 0; fail -1 
 */
int cfg_init(char filepath[], unsigned int groupIpArr[], int *groupNum);
DWORD WINAPI terminal_command(LPVOID lpdwThreadParam);

#endif
