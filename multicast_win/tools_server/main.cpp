#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>

#define DATA_BUF_LEN 1024 
#define SERVER_DEST_PORT 65000

int port_init();

int main(int argc, char** argv)
{
    char dest_ip[16] = "127.0.0.1";
    unsigned short dest_port = port_init();
    //printf("dest port:%d\n", dest_port);
    WORD wVersionReq;
    //set version 
    wVersionReq = MAKEWORD(2, 2);
    //init socket 
    int rt;
    WSADATA wsaData;
    rt = WSAStartup(wVersionReq, &wsaData); 
    if(rt != 0) 
    {
        perror("WSAStartup");	
    }

    if(LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
    {
        WSACleanup();
        perror("Version error");	
    }

    SOCKADDR_IN addrSrv;
    addrSrv.sin_addr.S_un.S_addr = inet_addr(dest_ip);
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(dest_port);

    char buf[DATA_BUF_LEN], buf_rcv[DATA_BUF_LEN];
    int len = sizeof(addrSrv);
    int reload_len = strlen("reload");
    int list_len = strlen("list");
    while(1)
    {
        printf("please input command[reload or list]#");
        memset(buf_rcv, 0, DATA_BUF_LEN);
		scanf("%s", buf);
        SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if(sockSrv == INVALID_SOCKET)
        {
            perror("socket");
            return -1;
        }

        if (connect(sockSrv, (SOCKADDR *)&addrSrv, len) == SOCKET_ERROR)
        {
            perror("connect");
            return -1;
        }

        int n = send(sockSrv, buf, strlen(buf), 0);
        if(n < 0)
        {
            perror("n<0");
        }
        else
        {
            printf("send %s[yes]\n", buf);
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
                    printf("recv_pkt:\t%d\ndrop_pkt:\t%d\nforward_pkt:\t%d\nfwd_point_pkt:\t%d\n", recv_pkt, drop_pkt, forward_pkt, forward_point_pkt);
                }
                else
                {
                    printf("%s\n", buf_rcv);
                }
            }
        }
        memset(buf, 0, DATA_BUF_LEN); 
        Sleep(1000);
        closesocket(sockSrv);
    }
    WSACleanup(); 
    return 0;
}

int port_init()
{
    int port;
    FILE *fp = fopen("./server_port.txt", "r"); 
    if(fp == NULL)
    {
        return SERVER_DEST_PORT;
    }
    fscanf(fp, "%d", &port);
    if(port > 65536 || port < 0)
    {
        return SERVER_DEST_PORT;
    }
    return port;
}
