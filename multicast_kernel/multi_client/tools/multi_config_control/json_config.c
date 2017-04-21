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

    if((access(MULTI_CLIENT_CONFIG_FILE, F_OK)) == -1)
        return NULL;

    root_json = json_object_from_file(MULTI_CLIENT_CONFIG_FILE);
    if(root_json == NULL){
        //printf("json_object_from_file error\n");
        return NULL;
    }   

    grp_array = json_object_object_get(root_json, MULTI_GROUP_ARRAY);
    if(grp_array == NULL){
        //printf("json_object_object_get error\n");
        return NULL;
    }   

    if ( !json_object_is_type(grp_array, json_type_array)){
        //printf("json format wrong!\n");
        return NULL;
    }   

    *root = root_json;
    return grp_array;
}


int multi_client_config_recover(void)
{
    int i, num, port, ret = 0;
    const char *multi_ip, *server_ip;
    struct json_object *root_json, *grp_array, *grp_obj;
    char cmd[1024] = {0};

    grp_array = json_get_multi_group_array(&root_json);
    if(grp_array == NULL)
        return -1;

    num = json_object_array_length(grp_array);
    for(i=0; i<num; i++){
        grp_obj = json_object_array_get_idx(grp_array, i);
        VALUE_IS_NULL_PRINT_AND_GOTO_FAIL(grp_obj, "json format wrong", 1);

        multi_ip = json_object_get_string(json_object_object_get(grp_obj, MULTI_GROUP_IP));
        VALUE_IS_NULL_PRINT_AND_GOTO_FAIL(multi_ip, "json format wrong", 1);

        server_ip = json_object_get_string(json_object_object_get(grp_obj, MULTI_SERVER_IP));
        VALUE_IS_NULL_PRINT_AND_GOTO_FAIL(server_ip, "json format wrong", 1);

        port = json_object_get_int(json_object_object_get(grp_obj, MULTI_CLIENT_PORT));

        sprintf(cmd, "%s -A -m %s -i %s -p %d",
                MULTI_CLIENT_SHELL_CMD, multi_ip, server_ip, port);

        system(cmd);
    } 

FAIL:
        json_object_to_file_ext(MULTI_CLIENT_CONFIG_FILE, root_json, JSON_C_TO_STRING_PRETTY);
        return ret;
}
