#include <stdio.h>
#include <winsock2.h>
#include "json.h"
#include "multi_grp.h"
#include "config.h"
#include "handleIncomeData.h" 

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
    struct json_object *json_root;

    json_root = json_object_from_file(filepath);

    if(json_root == (struct json_object *)(-1))
    {
        fprintf(gfp_log, "[%s:%d]json_root is null\n", __FILE__, __LINE__);
        return -1;
    }

    struct json_object *grp_array = 
        json_object_object_get(json_root, "multi_group_array");
    if(grp_array == NULL)
    {
        fprintf(gfp_log, "[%s:%d]json_object_object_get\n", __FILE__, __LINE__);
        return -1;
    } 

    if ( !json_object_is_type(grp_array, json_type_array))
    {
        fprintf(gfp_log, "[%s:%d]json_object_is_type\n", __FILE__, __LINE__);
        return -1;
    }

    //printf("my_object.to_string()=%s\n", json_object_to_json_string(json_root));
    int num = json_object_array_length(grp_array);
    *groupNum = num;
    int i;
    struct json_object *grp_obj, *ip_list;
    const char *group_ip;
    unsigned int group_port, server_port;
    printf("num: %d\n", num);
    memset(gbpf_filter_string, 0, FILTER_STRING_SIZE);
    for(i=0; i<num; i++)
    {
        grp_obj = json_object_array_get_idx(grp_array, i);
        group_ip = 
            json_object_get_string(json_object_object_get(grp_obj, "group_ip"));
        group_port = 
            json_object_get_int(json_object_object_get(grp_obj, "group_port"));
        server_port = 
            json_object_get_int(json_object_object_get(grp_obj, "server_port"));

        groupIpArr[i] = inet_addr(group_ip);
        printf("gip:%s gport:%u sport:%u\n", group_ip, group_port, server_port);

        if(i==0)
        {
            sprintf(gbpf_filter_string + strlen(gbpf_filter_string), "port %u", group_port);
        }
        else
        {
            sprintf(gbpf_filter_string + strlen(gbpf_filter_string), " or port %u", group_port);
        }

        ip_list = json_object_object_get(grp_obj, "member_array");
        int num_ip = json_object_array_length(ip_list);
        printf("ip_list_num:%d\n", num_ip);

        uint32_t ip_list_int[MAX_MEM_IP_IN_GROUP];
        for(int j=0; j<num_ip; j++)
        {
            ip_list_int[j] = inet_addr(json_object_get_string(json_object_array_get_idx(ip_list, j)));
            printf("%s\n", inet_ntoa(*(struct in_addr *)&ip_list_int[j]));
        }
        printf("\n"); 

        add_multi_node(inet_addr(group_ip), 
                group_port, 
                server_port, 
                ip_list_int, 
                num_ip);
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
DWORD WINAPI terminal_command(LPVOID lpdwThreadParam)
{
    unsigned short dest_port = reload_cfg_port_init();

    SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sockSrv == INVALID_SOCKET)
    {
        fprintf(gfp_log, "[%s:%d]socket error!\n", __FILE__, __LINE__);
        return -1;
    }

    SOCKADDR_IN addrSrv;
    addrSrv.sin_addr.S_un.S_addr = INADDR_ANY;
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(dest_port);

    int len = sizeof(addrSrv);

    if ((bind(sockSrv, (SOCKADDR *)&addrSrv, len)) == SOCKET_ERROR)
    {
        fprintf(gfp_log, "[%s:%d]bind error!\n", __FILE__, __LINE__);
        return -1;
    }
#define MAX_CONN 10
    if (listen(sockSrv, MAX_CONN) == SOCKET_ERROR)
    {
        fprintf(gfp_log, "[%s:%d]listen error!\n", __FILE__, __LINE__);
        return -1;
    }

    int reload_len = strlen("reload");
    int list_len = strlen("list");
    while(1)
    {
        SOCKET clientSocket = accept(sockSrv, 0, 0);
        if(clientSocket == INVALID_SOCKET)
        {
            fprintf(gfp_log, "[%s:%d]accept error!\n", __FILE__, __LINE__);
            return -1;
        }

        int iResult, recvBufLen = BUFSIZE;
        char recvBuf[BUFSIZE]; 
        do
        {
            memset(recvBuf, 0, recvBufLen); 
            iResult = recv(clientSocket, recvBuf, recvBufLen, 0);
            printf("recv_len:%d\n", iResult);
            if(iResult > 0)
            {
                if(iResult == reload_len && strncmp(recvBuf, "reload", reload_len) == 0)
                {
                    printf("rev reload yes!\n");
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
                        if (nRet == SOCKET_ERROR) 
                        {
                            fprintf(gfp_log, "[%s:%d]setsockopt() IP_ADD_MEMBERSHIP address %s failed, Err: %d\n", __FILE__, __LINE__, inet_ntoa(*(struct in_addr *)&groupIp[i]), WSAGetLastError());
                        }
                    }

                    //hash init
                    multi_node_init();
                    int rt = cfg_init(SERVER_CFG, groupIp, &groupNum);
                    if(rt != 0)
                    {
                        fprintf(gfp_log, "[%s:%d]cfg_init error!\n", __FILE__, __LINE__);
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
                        if (nRet == SOCKET_ERROR) 
                        {
                            fprintf(gfp_log, "[%s:%d]setsockopt() IP_ADD_MEMBERSHIP address %s failed, Err: %d\n", __FILE__, __LINE__, inet_ntoa(*(struct in_addr *)&groupIp[i]), WSAGetLastError());
                        }
                    }

                    system("C:\\udptopoint\\udptopoint_reload.bat");
                }
                else if(iResult == list_len && strncmp(recvBuf, "list", list_len) == 0)
                {
                    printf("rev list yes!\n");
                    //add the data to send
                    char list_buf[BUFSIZE];
                    memset(list_buf, 0, BUFSIZE);
                    //printf("%d %d %d\n", grecv_pkt, gdrop_pkt, gforward_pkt);
                    sprintf(list_buf, "%d %d %d %d", grecv_pkt, gdrop_pkt, gforward_pkt, gforward_point_pkt);
                    printf("list_buf:%s\n", list_buf);
                    if(send(clientSocket, list_buf, strlen(list_buf), 0) < 0)
                    {
                        fprintf(gfp_log, "[%s:%d]send Err: %d\n", __FILE__, __LINE__, WSAGetLastError());
                    }
                }
                else
                {
                    printf("parameter error[%s]\n", recvBuf); 
                    if(send(clientSocket, "parameter error!\n", strlen("parameter error!\n"), 0) < 0)
                    {
                        fprintf(gfp_log, "[%s:%d]send Err: %d\n", __FILE__, __LINE__, WSAGetLastError());
                    }
                }
            }
            else if(iResult == 0)
            {
                printf("connection closing...\n");
            }
            else
            {
                fprintf(gfp_log, "[%s:%d]recv error!\n", __FILE__, __LINE__);
                return -1;
            }
        }while(iResult > 0);

        closesocket(clientSocket);
        Sleep(1000);
    }
    closesocket(sockSrv);
    return 0;
}
