#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include <string.h>
#include <errno.h>
#include "json.h" 
#include "multi_grp.h"
#include "macro_define.h"
#include "handleData.h"
#include "connectInit.h"
#include "log.h"
/*
 * func:  load the configuration and add them to the hash
 * param£ºfilepath the location of file
 * return: success 0; fail -1 
 */
int cfg_init(char *filepath)
{
    struct json_object *json_root;

    json_root = json_object_from_file(filepath);
    if(json_root == (struct json_object *)(void *)(-1))
    {
        fprintf(gfp_log, "[%s:%d]json_root is null\n", __FILE__, __LINE__);
        fflush(gfp_log);
        return -1;
    }

    struct json_object *grp_array; 
    json_object_object_get_ex(json_root, "multi_server_info_array",&grp_array);
    if(grp_array == NULL)
    {
        fprintf(gfp_log, "[%s:%d]json_object_object_get\n", __FILE__, __LINE__);
        fflush(gfp_log);
        return -1;
    } 

    if (!json_object_is_type(grp_array, json_type_array))
    {
        fprintf(gfp_log, "[%s:%d]json_object_is_type\n", __FILE__, __LINE__);
        fflush(gfp_log);
        return -1;
    }

    //printf("my_object.to_string()=%s\n", json_object_to_json_string(json_root));
    int num = json_object_array_length(grp_array);
    int i, rt;
    struct json_object *grp_obj;
    const char *server_ip, *group_ip;
    unsigned int server_port, group_port;

    fprintf(gfp_log, "num: %d\n", num);
    struct json_object * json_tmp;
    for(i=0; i<num; i++)
    {
        grp_obj = json_object_array_get_idx(grp_array, i);
	json_object_object_get_ex(grp_obj,"server_ip",&json_tmp);
        server_ip = json_object_get_string(json_tmp);
        json_object_object_get_ex(grp_obj,"server_port",&json_tmp);
        server_port = json_object_get_int(json_tmp);
	json_object_object_get_ex(grp_obj,"group_ip",&json_tmp);
        group_ip = json_object_get_string(json_tmp);
	json_object_object_get_ex(grp_obj,"group_port",&json_tmp);
        group_port = json_object_get_int(json_tmp);

        fprintf(gfp_log, "sip:%s sport:%u\n", server_ip, server_port);
        fprintf(gfp_log, "gip:%s gport:%u\n\n", group_ip, group_port);

        /* add the cfg to the hash table */
        rt = add_multi_node(inet_addr(group_ip), group_port, inet_addr(server_ip), server_port);
        if(rt != 0)
        {
            fprintf(gfp_log, "[%s:%d]add_multi_node is wrong!\n", __FILE__, __LINE__);
        }
    }
    fflush(gfp_log);
    return 0;
}
int json_add_grp_node(char serverip[] , uint32_t groupport , uint32_t serverport , char multi_ip[])
{
    struct json_object *json_root = NULL;
    json_root = json_object_from_file(CLIENT_CFG);
    if( json_root == NULL )
    {
    	json_root = json_object_new_object();
    }
    struct json_object *grp_array = NULL;
    json_object_object_get_ex(json_root, "multi_server_info_array", &grp_array);
    if(grp_array == NULL)
    {
	grp_array = json_object_new_array();
	json_object_object_add(json_root , "multi_server_info_array" , grp_array);
    }
    if ( !json_object_is_type(grp_array, json_type_array))
    {
        return -1;
    }
    //printf("my_object.to_string()=%s\n", json_object_to_json_string(json_root));
    int num_array = json_object_array_length(grp_array);
    //*groupNum = num_array;
    int i;
    struct json_object *grp_obj;
    struct json_object *value_tmp;
    const char *group_ip;
    const char *server_ip;
    unsigned int group_port, server_port;
    for(i=0; i<num_array; i++)
    {
        grp_obj = json_object_array_get_idx(grp_array, i);
        json_object_object_get_ex(grp_obj , "server_ip" , &value_tmp);
        server_ip = json_object_get_string(value_tmp);
        json_object_object_get_ex(grp_obj , "group_ip" , &value_tmp);
        group_ip = json_object_get_string(value_tmp);
        json_object_object_get_ex(grp_obj , "group_port" , &value_tmp);
        group_port = json_object_get_int(value_tmp);	
        json_object_object_get_ex(grp_obj , "server_port" , &value_tmp);
        server_port = json_object_get_int(value_tmp); 	
        if(strcmp(server_ip , serverip) || groupport != group_port)
                break;
    }
    if(i<num_array || num_array == 0)//add ip member to multicast ip
    {
	json_object * new_json_grp_node = json_object_new_object();
	json_object_object_add(new_json_grp_node , "server_ip" , json_object_new_string(serverip));
	json_object_object_add(new_json_grp_node , "server_port" , json_object_new_int(serverport));
	json_object_object_add(new_json_grp_node , "group_port" , json_object_new_int(groupport));
	json_object_object_add(new_json_grp_node , "group_ip" , json_object_new_string(multi_ip));
	json_object_array_add(grp_array,new_json_grp_node);
        json_object_to_file(CLIENT_CFG, json_root);
    }else{//add all multicast node
    }
    return 0;
}
int json_del_grp_node(char serverip[] , uint32_t groupport)
{
    struct json_object *json_root = NULL;
    json_root = json_object_from_file(CLIENT_CFG);
    if( json_root == NULL)
    {
        return -1;
    }
    struct json_object *grp_array;
    json_object_object_get_ex(json_root, "multi_server_info_array", &grp_array);
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
    struct json_object *grp_obj;
    struct json_object *value_tmp;
    const char *group_ip;
    const char *server_ip;
    unsigned int group_port, server_port;
    json_object * new_json_grp = json_object_new_array();
    for(i=0; i<num_array; i++)
    {
        grp_obj = json_object_array_get_idx(grp_array, i);
        json_object_object_get_ex(grp_obj , "server_ip" , &value_tmp);
        server_ip = json_object_get_string(value_tmp);
        json_object_object_get_ex(grp_obj , "group_ip" , &value_tmp);
        group_ip = json_object_get_string(value_tmp);
        json_object_object_get_ex(grp_obj , "group_port" , &value_tmp);
        group_port = json_object_get_int(value_tmp);	
	json_object_object_get_ex(grp_obj , "server_port" , &value_tmp);
	server_port = json_object_get_int(value_tmp);
        if(strcmp(server_ip , serverip) || groupport != group_port)
	{
	    json_object * new_json_grp_node = json_object_new_object();
	    json_object_object_add(new_json_grp_node , "server_ip" ,json_object_new_string(server_ip));
	    json_object_object_add(new_json_grp_node , "server_port" , json_object_new_int(server_port));
            json_object_object_add(new_json_grp_node , "group_port" , json_object_new_int(group_port));
	    json_object_object_add(new_json_grp_node , "group_ip" , json_object_new_string(group_ip));
	    json_object_array_add(new_json_grp , new_json_grp_node);
	}
    }
    json_object_object_del(json_root , "multi_server_info_array");
    json_object_object_add(json_root , "multi_server_info_array" , new_json_grp);
    json_object_to_file(CLIENT_CFG , json_root);
    return 0;
}
int json_clear_grp_node()
{
    struct json_object *json_root = NULL;
    json_root = json_object_from_file(CLIENT_CFG);
    if( json_root == NULL)
    {
        return -1;
    }
    struct json_object *grp_array = NULL;
    json_object_object_get_ex(json_root, "multi_server_info_array", &grp_array);
    if(grp_array == NULL)
    {
        return -1;
    }
    if ( !json_object_is_type(grp_array, json_type_array))
    {
        return -1;
    }
    json_object_object_del(json_root , "multi_server_info_array");
    json_object * new_json_grp = json_object_new_array();
    json_object_object_add(json_root , "multi_server_info_array" , new_json_grp);
    json_object_to_file(CLIENT_CFG , json_root);
    return 0;
}

int json_list_multicast()
{
    struct json_object * json_root = NULL;
    json_root = json_object_from_file(CLIENT_CFG);
    if( json_root == NULL )
    {
        return -1;
    }
    struct json_object * grp_array = NULL;
    json_object_object_get_ex(json_root , "multi_server_info_array" , &grp_array);
    if(grp_array == NULL)
    {
       return -1;
    }
    int array_num = json_object_array_length(grp_array);
    struct json_object * grp_obj;
    struct json_object * grp_ip;
    struct json_object * grp_port;
    struct json_object * ser_ip;
    const char * group_ip;
    const char * server_ip;
    int group_port;
    int i = 0 ;
    if( array_num == 0 )
    {
         printf("Not configure multicast server ip and port!\n");
    }
    else{
    
    	for( i = 0 ; i != array_num ; i++ )
    	{
		printf("Configure Multicast Server %d:\n",i+1);
        	grp_obj = json_object_array_get_idx(grp_array , i);
		json_object_object_get_ex(grp_obj , "server_ip" , &ser_ip);
		server_ip = json_object_get_string(ser_ip);
        	json_object_object_get_ex(grp_obj , "group_ip" , &grp_ip);
        	group_ip = json_object_get_string(grp_ip);
		json_object_object_get_ex(grp_obj , "group_port" , &grp_port);
		group_port = json_object_get_int(grp_port);
		printf("Server IP:  %s\n" , server_ip);
		printf("UDP Port:  %d\n" , group_port);
		printf("Multicast  IP:  %s\n" , group_ip);
		printf("\n\n");
    	}
    }
    return 0;
} 
/*
 * func:    the port of receiving terminal command init
 * param:   
 * return:  
 */
int reload_cfg_port_init()
{
    int port;
    FILE *fp = fopen(CLIENT_PORT_INIT_PATH, "r"); 
    if(fp == NULL)
    {
        return CLIENT_CFG_RELOAD_PORT;
    }
    fscanf(fp, "%d", &port);
    if(port > 65536 || port < 0)
    {
        return CLIENT_CFG_RELOAD_PORT;
    }
    return port;
}

/*
 * func:    reload the cfg
 * param:   
 * return:  
 */
void * cfg_reload(void * lpdwThreadParam)
{
    unsigned short dest_port = reload_cfg_port_init();

    SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sockSrv == -1)
    {
        fprintf(gfp_log, "[%s:%d]scoket error!\n", __FILE__, __LINE__);
        fflush(gfp_log);
        return NULL;
    }

    struct sockaddr_in addrSrv;
    addrSrv.sin_addr.s_addr = INADDR_ANY;
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(dest_port);

    int len = sizeof(addrSrv);

    if ((bind(sockSrv, (struct sockaddr *)&addrSrv, len)) == -1)
    {
        fprintf(gfp_log, "[%s:%d]bind error\n", __FILE__, __LINE__);
        fflush(gfp_log);
        return NULL;
    }
#define MAX_CONN 10
    if (listen(sockSrv, MAX_CONN) == -1)
    {
        fprintf(gfp_log, "[%s:%d]connect error!\n", __FILE__, __LINE__);
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
            fprintf(gfp_log, "[%s:%d]accept", __FILE__, __LINE__);
            fflush(gfp_log);
        }

        int iResult, recvBufLen = BUFSIZE;
        char recvBuf[BUFSIZE]; 
        do
        {
            iResult = recv(clientSocket, recvBuf, recvBufLen, 0);
            if(iResult > 0)
            {
                if(iResult == reload_len && strncmp(recvBuf, "reload", reload_len) == 0)
                {
                    multi_node_init(); 
                    int rt = cfg_init(CLIENT_CFG);
                    if(rt != 0)
                    {
                        fprintf(gfp_log, "[%s:%d]cfg_init error!\n", __FILE__, __LINE__);
                    }
                }
                else if(iResult == list_len && strncmp(recvBuf, "list", list_len) == 0)
                {
                    //add the data to send
                    char list_buf[BUFSIZE];
                    memset(list_buf, 0, BUFSIZE);
                    //printf("%d %d %d\n", grecv_pkt, gdrop_pkt, gforward_pkt);
                    sprintf(list_buf, "%d %d %d", grecv_pkt, gdrop_pkt, gforward_pkt);
                    if(send(clientSocket, list_buf, strlen(list_buf), 0) < 0)
                    {
                        fprintf(gfp_log, "[%s:%d]send Err: %d\n", __FILE__, __LINE__, errno);
                    }
                }
                else
                {
                    if(send(clientSocket, "parameter error!\n", strlen("parameter error!\n"), 0) < 0)
                    {
                        fprintf(gfp_log, "[%s:%d]send Err: %d\n", __FILE__, __LINE__, errno);
                    }
                }
            }
            if(iResult != 0)
            {
                fprintf(gfp_log, "[%s:%d]recv\n", __FILE__, __LINE__);
                fflush(gfp_log);
            }
        }while(iResult > 0);

        fflush(gfp_log);
        close(clientSocket);
    }
    close(sockSrv);
    return 0;
}
int reload_json_file()
{
     char dest_ip[16] = "127.0.0.1";
     unsigned short dest_port = reload_cfg_port_init();
     struct sockaddr_in addrSrv;
     addrSrv.sin_addr.s_addr = inet_addr(dest_ip);
     addrSrv.sin_family = AF_INET;
     addrSrv.sin_port = htons(dest_port);

     char buf[BUFSIZE], buf_rcv[BUFSIZE];
     int len = sizeof(addrSrv);
     memset(buf_rcv, 0, BUFSIZE);
     memcpy(buf , "reload", sizeof("reload"));
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
     memset(buf, 0, BUFSIZE);
     close(sockSrv);
     return 0;     
}
int get_drop_stats()
{
    char dest_ip[16] = "127.0.0.1";
    unsigned short dest_port = reload_cfg_port_init();
    struct sockaddr_in addrSrv;
    addrSrv.sin_addr.s_addr = inet_addr(dest_ip);
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(dest_port);

    char buf[BUFSIZE], buf_rcv[BUFSIZE];
    int len = sizeof(addrSrv);
    int reload_len = strlen("reload");
    int list_len = strlen("list");
    memset(buf_rcv, 0, BUFSIZE);
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
            n = recv(sockSrv, buf_rcv, BUFSIZE, 0);
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
                    int recv_pkt, drop_pkt, forward_pkt;
                    sscanf(buf_rcv, "%d %d %d", &recv_pkt, &drop_pkt, &forward_pkt);
                    //printf("recv_pkt:\t%d\ndrop_pkt:\t%d\nforward_pkt:\t%d\nfwd_point_pkt:\t%d\n", recv_pkt, drop_pkt, forward_pkt, forward_point_pkt);
                    printf("rx packets:%d\ntx packages:%d\n",  recv_pkt, forward_pkt);
                }
                else
                {
                    printf("%s\n", buf_rcv);
                }
            }
     }
     memset(buf, 0, BUFSIZE);
     close(sockSrv);
     return 0;

}
