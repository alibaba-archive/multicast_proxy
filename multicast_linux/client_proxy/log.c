#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
FILE *gfp_log;

void log_init(char *log_path)
{
    gfp_log = fopen(log_path, "w+");
    if(gfp_log == NULL)
    {
        printf("fopen is error! %s\n", strerror(errno));
    }
    else
    {
        fprintf(gfp_log, "log_init[ok]\n");
        fflush(gfp_log);
    } 
}

void log_uninit()
{
    fclose(gfp_log);
}
