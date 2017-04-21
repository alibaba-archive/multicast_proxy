#include <stdio.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <linux/netlink.h>
#include <unistd.h>
#include <fcntl.h>

#include "json.h"
#include "json_util.h"
#include "grp.h"
#include "multic_json_config.h"

struct json_object *json_get_multi_group_array(struct json_object **root)
{
    struct json_object *root_json, *grp_array;

    if((access(MULTI_SERVER_CONFIG_FILE, F_OK)) == -1)
        return NULL;

    root_json = json_object_from_file(MULTI_SERVER_CONFIG_FILE);
    if(root_json == NULL){
//        printf("json_object_from_file error\n");
        return NULL;
    }   

    grp_array = json_object_object_get(root_json, MULTI_GROUP_ARRAY);
    if(grp_array == NULL){
//        printf("json_object_object_get error\n");
        return NULL;
    }   

    if ( !json_object_is_type(grp_array, json_type_array)){
 //       printf("json format wrong!\n");
        return NULL;
    }   

    *root = root_json;
    return grp_array;
}


int multi_server_config_recover(void)
{
    int i, j, num, ip_num, ret = 0;
    const char *multi_ip, *mem_ip;
    struct json_object *root_json, *grp_array, *grp_obj, *mem_array;
    char cmd[4096] = {0};
    char *buf;

    grp_array = json_get_multi_group_array(&root_json);
    if(grp_array == NULL)
        return -1;

    num = json_object_array_length(grp_array);
    for(i=0; i<num; i++){
        grp_obj = json_object_array_get_idx(grp_array, i);
        VALUE_IS_NULL_PRINT_AND_GOTO_FAIL(grp_obj, "json format wrong", 1);

        multi_ip = json_object_get_string(json_object_object_get(grp_obj, MULTI_GROUP_IP));
        VALUE_IS_NULL_PRINT_AND_GOTO_FAIL(multi_ip, "json format wrong", 1);

        buf = cmd;
        buf += sprintf(buf, "%s -A -m %s", MULTI_SERVER_SHELL_CMD, multi_ip);

        mem_array = json_object_object_get(grp_obj, MULTI_MEMBER_ARRAY);
        VALUE_IS_NULL_PRINT_AND_GOTO_FAIL(mem_array, "json format wrong", 1);

        ip_num = json_object_array_length(mem_array);

        if(ip_num > 0)
            buf += sprintf(buf, " -j ");

        for(j = 0; j < ip_num; j++){
            mem_ip = json_object_get_string(json_object_array_get_idx(mem_array, j));
            buf += sprintf(buf, "%s,", mem_ip);
        }

        if(ip_num > 0)
            *(buf - 1) = '\0';

        system(cmd);
    } 

FAIL:
        json_object_to_file_ext(MULTI_SERVER_CONFIG_FILE, root_json, JSON_C_TO_STRING_PRETTY);
        return ret;
}
