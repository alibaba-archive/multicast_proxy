#include <stdio.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>    
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#include <pthread.h>
#include "connectInit.h"
#include "worker_thread.h"
#include "handleIncomeData.h"
#include "log.h"
/*
 * func:    work thread to deal with the coming data 
 * param:   
 * return:  
 */
void * WorkerThread (void * workcontext)
{  
    fd_set fds;
    struct timeval timeout = {1 , 0};
    int maxfd = g_hSocket + 1;
    char readbuf[BUFSIZE];
    int bytecount = 0;
    struct sockaddr_in addrfrom;
    addrfrom.sin_family = AF_INET;
    addrfrom.sin_addr.s_addr = htonl(INADDR_ANY);
    addrfrom.sin_port = htons(0);
    int addr_len = sizeof(struct sockaddr_in);
    while(1)
   {	
        FD_ZERO(&fds);
        FD_SET(g_hSocket ,&fds);
        int ret = select(maxfd,&fds,NULL,NULL,&timeout);
        switch(ret)
        {
            case -1 : //printf("select error\n"); break;
                    fprintf(gfp_log, "[%s:%d]select have some errors\n", __FILE__, __LINE__);
                    break; 
            case 0 : // printf("time out\n"); break;
		    //fprintf(gfp_log, "[%s:%d]select time out\n", __FILE__, __LINE__);
                    break;
            default:
                     if(FD_ISSET(g_hSocket , &fds))
                     {
                        bytecount = recvfrom(g_hSocket , readbuf , BUFSIZE , 0 ,(struct sockaddr*)&addrfrom ,(socklen_t*) &addr_len );    
		/*	printf("bytecount: %d \n",bytecount);
			printf("content: ");
			int in=0;
		        for(in=0;in!=bytecount;in++)
			{
			    printf("%c",readbuf[in]);
			}	
			printf("\n");*/
                        if (bytecount < 0)
                        {
                            //perror("recvfrom package error");
			    fprintf(gfp_log, "[%s:%d]recvfrom package error\n", __FILE__, __LINE__);
                        }
                        else{
                            HandleIncomingData(readbuf , bytecount ,1);
                        }
                     }
        }
 }       
}

/*
 * func:    create worker thead 
 * param:   dwNumberOfWorkers is the number of work thead.
 * return:  0 success; -1 fail
 */
int CreateWorkers (unsigned int number_of_workers)
{
    int i = 0;
    pthread_t thread_array[number_of_workers];

    for (i = 0; i < number_of_workers; i++)
    {
        int err = pthread_create (&thread_array[i],NULL, WorkerThread, NULL);
        if (err)
        {
            fprintf(gfp_log, "[%s:%d]Create Worker Thread %d  Failed\n", __FILE__, __LINE__,i);
            return -1;
        }

    }
    for(i=0; i< number_of_workers; i++)
    {
        pthread_join(thread_array[i],NULL);
    }

    return 0;
}
