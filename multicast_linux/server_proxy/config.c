#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include "json.h"
#include "multi_grp.h"
#include "config.h"
#include "macro_define.h"
#include "handleIncomeData.h"
#include "connectInit.h"
#define DATA_BUF_LEN 1024
#define SERVER_DEST_PORT 65000
unsigned int gmax_multi_ip, gmin_multi_ip;

/*
 * func:    the scope of multi ip
 * param:   
 * return:  
 */
void multi_ip_max_min_init()
{
    gmax_multi_ip = ntohl(inet_addr("239.255.255.255"));
    gmin_multi_ip = ntohl(inet_addr("224.0.0.0"));
}
/*
 * func:    load the configuration and add them to the hash
 * param:   filepath the location of file
 * return:  0 success; -1 fail 
 */
int cfg_init(char filepath[], uint32_t groupIpArr[], int *groupNum)
{
    struct json_object *json_root = NULL;
    json_root = json_object_from_file(filepath);

    if( json_root == NULL)
    {
         printf("server_cfg.json.txt have no data\n");
         return -1;
    }

    struct json_object *grp_array = NULL; 
    json_object_object_get_ex(json_root, "multi_group_array", &grp_array);
    if(grp_array == NULL)
    {
        return -1;
    } 

    if ( !json_object_is_type(grp_array, json_type_array))
    {
        return -1;
    }

    //printf("my_object.to_string()=%s\n", json_object_to_json_string(json_root));
    int num = json_object_array_length(grp_array);
    *groupNum = num;
    int i;
    struct json_object *grp_obj, *ip_list;
    struct json_object *value_tmp;
    const char *group_ip;
    unsigned int group_port, server_port;
    fprintf(gfp_log, "num: %d\n", num);
    for(i=0; i<num; i++)
    {
        grp_obj = json_object_array_get_idx(grp_array, i);
        json_object_object_get_ex(grp_obj , "group_ip" , &value_tmp);
        group_ip = 
            json_object_get_string(value_tmp);
        json_object_object_get_ex(grp_obj , "group_port" , &value_tmp);
        group_port = 
            json_object_get_int(value_tmp);
        json_object_object_get_ex(grp_obj , "server_port" , &value_tmp);
        server_port = 
            json_object_get_int(value_tmp);

        groupIpArr[i] = inet_addr(group_ip);
        fprintf(gfp_log, "gip:%s gport:%u sport:%u\n", group_ip, group_port, server_port);

        json_object_object_get_ex(grp_obj, "member_array",&ip_list);
        int num_ip = json_object_array_length(ip_list);
        fprintf(gfp_log, "ip_list_num:%d\n", num_ip);

        uint32_t ip_list_int[MAX_MEM_IP_IN_GROUP];
        int j=0;
        for(j=0; j<num_ip; j++)
        {
            ip_list_int[j] = inet_addr(json_object_get_string(json_object_array_get_idx(ip_list, j)));
            //fprintf(gfp_log, "%s\n", inet_ntoa(*(struct in_addr *)(&ip_list_int[j])));
        }
        fprintf(gfp_log, "\n"); 
        fflush(gfp_log);

        int tmp = add_multi_node(inet_addr(group_ip), 
                group_port, 
                server_port, 
                ip_list_int, 
                num_ip);
    }
    return 0;
}
int json_add_list(char multi_ip[], char ip_list[][32] , int num , int multi_server_port)
{
    struct json_object *json_root=NULL;
    json_root = json_object_from_file(SERVER_CFG);
    //printf("root: %d\n",json_root==NULL);
    if(json_root == NULL)
    {
        json_root = json_object_new_object();
    }
    struct json_object *grp_array = NULL; 
    json_object_object_get_ex(json_root, "multi_group_array", &grp_array);
    if(grp_array == NULL)
    {
        //fprintf(gfp_log, "[%s:%d]json_object_object_get\n", __FILE__, __LINE__);
        grp_array = json_object_new_array();
        json_object_object_add(json_root , "multi_group_array" ,grp_array);
       // return -1;
    } 
    if ( !json_object_is_type(grp_array, json_type_array))
    {
        return -1;
    }
    //printf("my_object.to_string()=%s\n", json_object_to_json_string(json_root));
    int num_array = json_object_array_length(grp_array);
    //*groupNum = num_array;
    int i;
    struct json_object *grp_obj, *member_ip_list;
    struct json_object *value_tmp;
    const char *group_ip;
    for(i=0; i<num_array; i++)
    {
        grp_obj = json_object_array_get_idx(grp_array, i);
        json_object_object_get_ex(grp_obj , "group_ip" , &value_tmp);
        group_ip = json_object_get_string(value_tmp);
        if(!strcmp(group_ip , multi_ip))
		break;
    }
    if(i<num_array)//add ip member to multicast ip
    {
        
        struct json_object * server_port_obj = NULL;
        server_port_obj = json_object_new_int(multi_server_port);	
        grp_obj = json_object_array_get_idx(grp_array , i);
	json_object_object_del(grp_obj , "server_port");
	json_object_object_add(grp_obj , "server_port" , server_port_obj);
        int it = 0;
	json_object_object_get_ex(grp_obj, "member_array",&member_ip_list);
//	json_object_object_del(grp_obj , "member_array");
	int member_num_ip = json_object_array_length(member_ip_list);
	for ( it = 0 ; it != num ; it++)
	{
            int tt = 0;
            for( tt = 0 ; tt != it ; tt++ )
	    {
	        if( !strcmp(ip_list[it] , ip_list[tt]))
		{
		    break;
		}
	    }
	    int jt = 0;
	    for( jt = 0 ; jt != member_num_ip ; jt++ )
	    {
	        if(!strcmp(ip_list[it] , json_object_get_string(json_object_array_get_idx(member_ip_list , jt))))
		{
                   break;
		}
	    }
            if( tt == it && jt == member_num_ip)
            {            
                json_object * new_ip_obj = json_object_new_string(ip_list[it]);
                json_object_array_add(member_ip_list,new_ip_obj);
             }
	}
	json_object_to_file(SERVER_CFG, json_root);
    }else{//add all multicast node
        json_object * new_json_ip_list = json_object_new_array();
	for ( i = 0 ; i != num ; i++)
	{
            json_object_array_add(new_json_ip_list , json_object_new_string(ip_list[i]));

	}
       json_object * new_json_group = json_object_new_object();
       json_object_object_add(new_json_group , "group_ip" , json_object_new_string(multi_ip));
       json_object_object_add(new_json_group , "group_port" , json_object_new_int(7127));
       json_object_object_add(new_json_group , "server_port" , json_object_new_int(multi_server_port));
       json_object_object_add(new_json_group , "member_array" , new_json_ip_list);
       json_object_array_add(grp_array , new_json_group);
       json_object_to_file(SERVER_CFG , json_root);
    }
    return 0;
}


int json_del_list(char multi_ip[], char ip_list[][32] , int num)
{
    struct json_object *json_root = NULL;
    json_root = json_object_from_file(SERVER_CFG);
    if(json_root == NULL)
    {
        printf("have no json data,please check\n");
        return -1;
    }
    struct json_object *grp_array = NULL; 
    json_object_object_get_ex(json_root, "multi_group_array", &grp_array);
    if(grp_array == NULL)
    {
        return -1;
    } 
    if ( !json_object_is_type(grp_array, json_type_array))
    {
        return -1;
    }
    //printf("my_object.to_string()=%s\n", json_object_to_json_string(json_root));
    int num_array = json_object_array_length(grp_array);
    //*groupNum = num_array;
    int i;
    struct json_object *grp_obj, *member_ip_list;
    struct json_object *value_tmp;
    const char *group_ip;
    for(i=0; i<num_array; i++)
    {
        grp_obj = json_object_array_get_idx(grp_array, i);
        json_object_object_get_ex(grp_obj , "group_ip" , &value_tmp);
        group_ip = json_object_get_string(value_tmp);
        if(!strcmp(group_ip , multi_ip))
		break;
    }
    if(i<num_array)//add ip member to multicast ip
    {   
        grp_obj = json_object_array_get_idx(grp_array , i);
        int it = 0;
	int jt = 0;
	json_object_object_get_ex(grp_obj, "member_array",&member_ip_list);
//	json_object_object_del(grp_obj , "member_array");
	int member_num_ip = json_object_array_length(member_ip_list);
        json_object * new_json_ip_list = json_object_new_array();
	int * pmember_exist_flag = (int *)malloc(member_num_ip * sizeof(int));
        memset(pmember_exist_flag , 0 , member_num_ip * sizeof(int));
        for( it = 0 ; it != num ; it ++)
	{
	    for ( jt = 0 ; jt != member_num_ip ; jt ++)
	    {
	       if(!strcmp(ip_list[it] , json_object_get_string(json_object_array_get_idx(member_ip_list , jt))))
	       {
	           pmember_exist_flag[jt] = 1;
	       }
	    }
	}
       for( jt = 0 ; jt != member_num_ip ; jt ++)
       {
          if(pmember_exist_flag[jt] == 0)
	  {
	     json_object * new_ip = json_object_new_string(json_object_get_string(json_object_array_get_idx(member_ip_list , jt)));
	     json_object_array_add(new_json_ip_list , new_ip);
	  }
       }

	json_object_object_del(grp_obj , "member_array");
	json_object_object_add(grp_obj , "member_array" , new_json_ip_list);
	json_object_to_file(SERVER_CFG, json_root);
	free(pmember_exist_flag);
    }else{//add all multicast node
        printf("multicast group ip does not exist\n");
	return -1;
    }
    return 0;
}

int json_clear_list()
{
    struct json_object *json_root = NULL;
    json_root = json_object_from_file(SERVER_CFG);
    if(json_root == NULL)
    {
        return -1;
    }
    json_object_object_del(json_root , "multi_group_array");
    struct json_object * grp_array = NULL;
    grp_array = json_object_new_array();
    json_object_object_add(json_root , "multi_group_array" , grp_array);
/*
    struct json_object *grp_array = NULL; 
    json_object_object_get_ex(json_root, "multi_group_array", &grp_array);
    if(grp_array == NULL)
    {
        return -1;
    } 
    if ( !json_object_is_type(grp_array, json_type_array))
    {
        return -1;
    }
    //printf("my_object.to_string()=%s\n", json_object_to_json_string(json_root));
    int num_array = json_object_array_length(grp_array);
    *groupNum = num_array;
    int i;
    struct json_object *grp_obj, *member_ip_list;
    struct json_object *json_tmp;
    struct json_object *value_tmp;
    const char *group_ip;
    unsigned int group_port, server_port;
    for(i=0; i<num_array; i++)
    {
        grp_obj = json_object_array_get_idx(grp_array, i);
        json_object_object_del(grp_obj, "member_array");
        json_object * new_member_list = json_object_new_array();
        json_object_object_add(grp_obj , "member_array" , new_member_list);	
    }*/
    json_object_to_file(SERVER_CFG, json_root);
    return 0;
}
int json_del_multicast(char multi_ip [])
{
    struct json_object *json_root = NULL;
    json_root = json_object_from_file(SERVER_CFG);
    if( json_root == NULL )
    {
        return -1;
    }
    struct json_object *grp_array = NULL; 
    json_object_object_get_ex(json_root, "multi_group_array", &grp_array);
    json_object * new_group_array = json_object_new_array();
    if(grp_array == NULL)
    {
        return -1;
    } 
    if ( !json_object_is_type(grp_array, json_type_array))
    {
        return -1;
    }
    //printf("my_object.to_string()=%s\n", json_object_to_json_string(json_root));
    int num_array = json_object_array_length(grp_array);
    //*groupNum = num_array;
    int i;
    struct json_object *grp_obj, *member_ip_list;
    struct json_object *value_tmp;
    const char *group_ip;
    for(i=0; i<num_array; i++)
    {
        grp_obj = json_object_array_get_idx(grp_array, i);
	json_object_object_get_ex(grp_obj , "group_ip" , &value_tmp);
	group_ip = json_object_get_string(value_tmp);
        json_object_object_get_ex(grp_obj , "member_array" , &member_ip_list);
	int mem_ip_num = json_object_array_length(member_ip_list);
        if(strcmp(group_ip , multi_ip))
	{    
             json_object * new_grp_obj = json_object_new_object();
             json_object_object_add(new_grp_obj , "group_ip" , json_object_new_string(group_ip));
             json_object_object_add(new_grp_obj , "group_port" , json_object_new_int(7127));
             json_object_object_add(new_grp_obj , "server_port" , json_object_new_int(7126));
             json_object * new_member_ip_list = json_object_new_array();
             int it = 0;
             for( it = 0 ; it != mem_ip_num ; it++ )
	     {
	         json_object_array_add(new_member_ip_list , json_object_new_string(json_object_get_string(json_object_array_get_idx(member_ip_list,it))));
	     }
             json_object_object_add(new_grp_obj , "member_array" , new_member_ip_list);	     
	     json_object_array_add(new_group_array , new_grp_obj);
	}
    }
    json_object_object_del(json_root , "multi_group_array");
    json_object_object_add(json_root, "multi_group_array" , new_group_array);//new_group_array);
    json_object_to_file(SERVER_CFG, json_root);
    return 0;
}

int json_show_multicast()
{
    struct json_object * json_root = NULL;
    json_root = json_object_from_file(SERVER_CFG);
    if( json_root == NULL)
    {
         printf("have no multicast group\n");
	 return -1;
    }
    struct json_object * grp_array = NULL;
    json_object_object_get_ex(json_root , "multi_group_array" , &grp_array);
    if( grp_array == NULL )
    {
         printf("have no multicast group\n");
	 return -1;
    }
    int num_array = json_object_array_length(grp_array);
    struct json_object * grp_obj;
    struct json_object * grp_ip;
    const char * grp_ip_str;
    int i = 0;
    printf("Multicast Group IP: \n");
    for( i = 0 ; i != num_array ; i++)
    {
         grp_obj = json_object_array_get_idx(grp_array , i);
	 json_object_object_get_ex(grp_obj , "group_ip" , &grp_ip);
	 grp_ip_str = json_object_get_string(grp_ip);
	 printf("%s\n", grp_ip_str);
    }
    return 1;
}

int json_list_multicast(char multi_ip [])
{
    json_object * json_root = NULL;
    json_root = json_object_from_file(SERVER_CFG);
    if( json_root == NULL )
    {
        printf("have no multicast group\n");
	return -1;
    }
    json_object * grp_array = NULL;
    json_object_object_get_ex( json_root , "multi_group_array" , &grp_array);
    if( grp_array == NULL )
    {
        printf("have no multicast group\n");
	return -1;
    }
    int num_array = json_object_array_length(grp_array);
    struct json_object * grp_obj;
    struct json_object * grp_ip;
    struct json_object * ip_list;
    const char * grp_ip_str;
    int i = 0 ;
    for( i = 0 ; i != num_array ; i++)
    {
        grp_obj =  json_object_array_get_idx(grp_array , i);
	json_object_object_get_ex(grp_obj , "group_ip" , &grp_ip);
	grp_ip_str = json_object_get_string(grp_ip);
	if( !strcmp(multi_ip , grp_ip_str) )
	{
            printf("Multicast IP:  %s\n",grp_ip_str);
	    json_object_object_get_ex(grp_obj , "member_array" , &ip_list);
	    int ip_num = json_object_array_length(ip_list);
	    int ip_i = 0;
	    json_object * ip_elem;
	    const char * ip_str;
            printf("VM IP List:  \n");
	    for(ip_i = 0 ; ip_i != ip_num ; ip_i ++)
	    {
	         ip_elem = json_object_array_get_idx(ip_list , ip_i);
		 ip_str = json_object_get_string(ip_elem);
		 printf("    %s\n",ip_str);
	         	 
	    }
	}
    }
    return 1;
}

/*
 * func:    the port of receiving terminal command init
 * param:   
 * return:  
 */
int reload_cfg_port_init()
{
    int port;
    FILE *fp = fopen(SERVER_PORT_INIT_PATH, "r"); 
    if(fp == NULL)
    {
        return SERVER_CFG_RELOAD_PORT;
    }
    fscanf(fp, "%d", &port);
    if(port > 65536 || port < 0)
    {
        return SERVER_CFG_RELOAD_PORT;
    }
    return port;
}

/*
 * func:    recive the command from terminal 
 * param:   
 * return:  
 */

void * terminal_command(void * lpdwThreadParam)
{
    unsigned short dest_port = reload_cfg_port_init();

    SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sockSrv == -1)
    {
        fprintf(gfp_log, "[%s:%d]socket error!\n", __FILE__, __LINE__);
        fflush(gfp_log); 
        return NULL;
    }

    struct sockaddr_in  addrSrv;
    addrSrv.sin_addr.s_addr= htonl(INADDR_ANY);
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(dest_port);

    int len = sizeof(addrSrv);

    if ((bind(sockSrv, (struct sockaddr *)&addrSrv, len)) == -1)
    {
        fprintf(gfp_log, "[%s:%d]bind error!\n", __FILE__, __LINE__);
	fflush(gfp_log);
        return NULL;
    }
#define MAX_CONN 10
    if (listen(sockSrv, MAX_CONN) == -1)
    {
        fprintf(gfp_log, "[%s:%d]listen error!\n", __FILE__, __LINE__);
	fflush(gfp_log);
        return NULL;
    }

    int reload_len = strlen("reload");
    int list_len = strlen("list");
    while(1)
    {
        SOCKET clientSocket = accept(sockSrv, 0, 0);
        if(clientSocket == -1)
        {
            fprintf(gfp_log, "[%s:%d]accept error!\n", __FILE__, __LINE__);
	    fflush(gfp_log);
            continue;
        }

        int iResult, recvBufLen = BUFSIZE;
        char recvBuf[BUFSIZE]; 
        do
        {
            memset(recvBuf, 0, recvBufLen); 
            iResult = recv(clientSocket, recvBuf, recvBufLen, 0);
            //printf("%d\n", iResult);
            if(iResult > 0)
            {
                if(iResult == reload_len && strncmp(recvBuf, "reload", reload_len) == 0)
                {
                    //printf("rev reload yes!\n");
                    //remove the multicast group
                    int i;
                    struct ip_mreq stMreq;
                    for(i=0; i<groupNum; i++)
                    {
                        if(ntohl(groupIp[i]) > gmax_multi_ip ||
                                ntohl(groupIp[i]) < gmin_multi_ip)
                        {
                            continue;
                        }
                        stMreq.imr_multiaddr.s_addr = groupIp[i];
                        stMreq.imr_interface.s_addr = htonl(INADDR_ANY);
                        int nRet = setsockopt(g_hSocket, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&stMreq, sizeof(stMreq));
                        if (nRet != 0) 
                        {
                            fprintf(gfp_log, "[%s:%d]setsockopt() IP_ADD_MEMBERSHIP address %s failed, Err: %d\n", __FILE__, __LINE__, inet_ntoa(*(struct in_addr *)&groupIp[i]), errno);
			    fflush(gfp_log);
                        }
                    }

                    //hash init
                    multi_node_init();
                    int rt = cfg_init(SERVER_CFG, groupIp, &groupNum);
                    if(rt != 0)
                    {
                        fprintf(gfp_log, "[%s:%d]cfg_init error!\n", __FILE__, __LINE__);
			fflush(gfp_log);
                    }
                    // Join the multicast group so we can receive from it 
                    for(i=0; i<groupNum; i++)
                    {
                        if(ntohl(groupIp[i]) > gmax_multi_ip ||
                                ntohl(groupIp[i]) < gmin_multi_ip)
                        {
                            continue;
                        }
                        stMreq.imr_multiaddr.s_addr = groupIp[i];
                        stMreq.imr_interface.s_addr = htonl(INADDR_ANY);
                        int nRet = setsockopt(g_hSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&stMreq, sizeof(stMreq));
                        if (nRet != 0) 
                        {
                            fprintf(gfp_log, "[%s:%d]setsockopt() IP_ADD_MEMBERSHIP address %s failed, Err: %s\n", __FILE__, __LINE__, inet_ntoa(*(struct in_addr *)&groupIp[i]), strerror(errno));
			    fflush(gfp_log);
                        }
                    }
                }
                else if(iResult == list_len && strncmp(recvBuf, "list", list_len) == 0)
                {
                    //printf("rev list yes!\n");
                    //add the data to send
                    char list_buf[BUFSIZE];
                    memset(list_buf, 0, BUFSIZE);
                    //printf("%d %d %d\n", grecv_pkt, gdrop_pkt, gforward_pkt);
                    sprintf(list_buf, "%d %d %d %d", grecv_pkt, gdrop_pkt, gforward_pkt, gforward_point_pkt);
                    if(send(clientSocket, list_buf, strlen(list_buf), 0) < 0)
                    {
                        fprintf(gfp_log, "[%s:%d]send Err: %d\n", __FILE__, __LINE__, errno);
			fflush(gfp_log);
                    }
                }
                else
                {
                    printf("parameter error[%s]\n", recvBuf); 
                    if(send(clientSocket, "parameter error!\n", strlen("parameter error!\n"), 0) < 0)
                    {
                        fprintf(gfp_log, "[%s:%d]send Err: %d\n", __FILE__, __LINE__, errno);
                        fflush(gfp_log);
                    }
                }
            }
            else if(iResult == 0)
            {
                //printf("connection closing...\n");
            }
            else
            {
                fprintf(gfp_log, "[%s:%d]recv error!\n", __FILE__, __LINE__);
            }
        }while(iResult > 0);

        close(clientSocket);
        sleep(1);
    }
    close(sockSrv);
}

int reload_json_port_init()
{
    int port;
    FILE *fp = fopen(SERVER_PORT_INIT_PATH, "r");
    if(fp == NULL)
    {
        //printf("open error");
        return SERVER_DEST_PORT;
    }
    fscanf(fp, "%d", &port);
    if(port > 65536 || port < 0)
    {
        return SERVER_DEST_PORT;
    }
    return port;
}
int reload_json_file()
{
    char dest_ip[16] = "127.0.0.1";
    unsigned short dest_port = reload_json_port_init();
    struct sockaddr_in addrSrv;
    addrSrv.sin_addr.s_addr = inet_addr(dest_ip);
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(dest_port);

    char buf[DATA_BUF_LEN], buf_rcv[DATA_BUF_LEN];
    int len = sizeof(addrSrv);
    memset(buf_rcv, 0, DATA_BUF_LEN);
    memcpy(buf , "reload", sizeof("reload")); 
    SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sockSrv == -1)
    {
            printf("socket init error\n");
            return -1;
    }

     if (connect(sockSrv, (struct sockaddr*)&addrSrv, len) == -1)
     {
            printf("socket connect error\n");
            return -1;
     }

     int n = send(sockSrv, buf, strlen(buf), 0);
     if(n < 0)
     {
         printf("sock send error\n");
         return -1;
     }    
     memset(buf, 0, DATA_BUF_LEN);
     close(sockSrv);
     return 0;
} 
int get_drop_stats()
{
    char dest_ip[16] = "127.0.0.1";
    unsigned short dest_port = reload_json_port_init();
    struct sockaddr_in addrSrv;
    addrSrv.sin_addr.s_addr = inet_addr(dest_ip);
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(dest_port);

    char buf[DATA_BUF_LEN], buf_rcv[DATA_BUF_LEN];
    int len = sizeof(addrSrv);
    int reload_len = strlen("reload");
    int list_len = strlen("list");
    memset(buf_rcv, 0, DATA_BUF_LEN);
    memcpy(buf , "list", sizeof("list"));
    SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sockSrv == -1)
    {
            return -1;
    }

     if (connect(sockSrv, (struct sockaddr*)&addrSrv, len) == -1)
     {
            return -1;
     }

     int n = send(sockSrv, buf, strlen(buf), 0);
     if(n < 0)
     {
         return -1;
     }
     if(strncmp(buf, "reload", reload_len) != 0)
      {
            n = recv(sockSrv, buf_rcv, DATA_BUF_LEN, 0);
            buf_rcv[n] = '\0';
            if(n < 0)
            {
                printf("recv from server is error!\n");
            }
            else
            {
                if(strlen(buf) == list_len && strncmp(buf, "list", list_len) == 0)
                {
                    //printf("recv: %d %s\n", n, buf);
                    int recv_pkt, drop_pkt, forward_pkt, forward_point_pkt;
                    sscanf(buf_rcv, "%d %d %d %d", &recv_pkt, &drop_pkt, &forward_pkt, &forward_point_pkt);
                    //printf("recv_pkt:\t%d\ndrop_pkt:\t%d\nforward_pkt:\t%d\nfwd_point_pkt:\t%d\n", recv_pkt, drop_pkt, forward_pkt, forward_point_pkt);
                    printf("rx packets:%d\ntx packages:%d\ndx packets:%d\n", recv_pkt,forward_point_pkt , drop_pkt);
                }
                else
                {
                    printf("%s\n", buf_rcv);
                }
            }
     }
     memset(buf, 0, DATA_BUF_LEN);
     close(sockSrv);
     return 0;

}














