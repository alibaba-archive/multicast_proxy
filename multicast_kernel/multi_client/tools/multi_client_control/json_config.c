#include <stdio.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <linux/netlink.h>

#include "json.h"
#include "json_util.h"
#include "grp.h"
#include "multic_json_config.h"

static const char *ip2str(uint32_t ip)  
{  
    struct in_addr a;  
    a.s_addr = htonl(ip);  
    return inet_ntoa(a);  
}  

struct json_object *json_get_multi_group_array(struct json_object **root)
{
    struct json_object *root_json, *grp_array;

    root_json = json_object_from_file(MULTI_CLIENT_CONFIG_FILE);
    if(root_json == NULL){
        printf("json_object_from_file error\n");
        return NULL;
    }

    grp_array = json_object_object_get(root_json, MULTI_GROUP_ARRAY);
    if(grp_array == NULL){
        printf("json_object_object_get error\n");
        return NULL;
    }

    if ( !json_object_is_type(grp_array, json_type_array)){
        printf("json format wrong!\n");
        return NULL;
    }

    *root = root_json;
    return grp_array;
}

static int __multi_grp_config_init(char * filepath)
{
    int ret;
    FILE * fp;
    struct json_object *json_root, *grp_array;

    //if config file is not exist, create it.
    fp=fopen(filepath,"a+");
    VALUE_IS_NULL_PRINT_AND_RETURN(fp, "open or create config file fail", 1);
    fclose(fp);

    json_root = json_object_from_file(filepath);

    ret = nf_json_config_root_init(&json_root);
    VALUE_IS_NEGATIVE_PRINT_AND_RETURN(ret, "init config file fail\n", 1);

    ret = nf_json_config_init_array(json_root, MULTI_GROUP_ARRAY, &grp_array);
    VALUE_IS_NEGATIVE_PRINT_AND_RETURN(ret, "init config file fail\n", 1);

    json_object_to_file_ext(filepath, json_root, JSON_C_TO_STRING_PRETTY);

    return 0;
}

int multi_grp_config_init(char * filepath)
{
    int ret;
    int fd;

    fd = nf_file_lock(filepath);
    VALUE_IS_NEGATIVE_PRINT_AND_RETURN(fd, "lock config file fail", 1);

    ret = __multi_grp_config_init(filepath);

    nf_file_unlock(fd);

    return ret;
}

int __multi_grp_add_to_config(struct tmcc_nl_service_st *grp)
{
    int i, num, port, ret = 0;
    const char *multi_ip, *server_ip, *new_multi_ip, *new_server_ip;
    struct json_object *root_json, *grp_array, *grp_obj;

    grp_array = json_get_multi_group_array(&root_json);
    if(grp_array == NULL)
        return -1;
        
    new_server_ip = ip2str(grp->server_ip);

    num = json_object_array_length(grp_array);
    if(num + 1 > MULTI_GRP_MAX){
        printf("error: you can only add %d multicast groups\n", MULTI_GRP_MAX);                  
        ret = -1;
        goto FAIL;
    }

    for(i=0; i<num; i++){
        grp_obj = json_object_array_get_idx(grp_array, i);
        VALUE_IS_NULL_PRINT_AND_GOTO_FAIL(grp_obj, "json format wrong", 1);

        multi_ip = json_object_get_string(json_object_object_get(grp_obj, MULTI_GROUP_IP));
        VALUE_IS_NULL_PRINT_AND_GOTO_FAIL(multi_ip, "json format wrong", 1);

        server_ip = json_object_get_string(json_object_object_get(grp_obj, MULTI_SERVER_IP));
        VALUE_IS_NULL_PRINT_AND_GOTO_FAIL(server_ip, "json format wrong", 1);

        port = json_object_get_int(json_object_object_get(grp_obj, MULTI_CLIENT_PORT));

        if((strcmp(server_ip, new_server_ip) == 0) && (port == grp->port)){
            new_multi_ip = ip2str(grp->multi_ip);
            if(strcmp(multi_ip, new_multi_ip) == 0){
                ret = -2;
                printf("Multicast group already exsit!\n");
                goto FAIL;
            }

            json_object_object_add(grp_obj, MULTI_GROUP_IP, json_object_new_string(new_multi_ip));
            break;
        }
    }

    if(i == num){  //the group not exist
        grp_obj = json_object_new_object();
        VALUE_IS_NULL_PRINT_AND_GOTO_FAIL(grp_obj, "json_object_new_object fail", 3);

        json_object_object_add(grp_obj, MULTI_SERVER_IP, json_object_new_string(new_server_ip));
        new_multi_ip = ip2str(grp->multi_ip);
        json_object_object_add(grp_obj, MULTI_GROUP_IP, json_object_new_string(new_multi_ip));
        json_object_object_add(grp_obj, MULTI_CLIENT_PORT, json_object_new_int(grp->port));
        json_object_array_add(grp_array, grp_obj);
    }

FAIL:
    json_object_to_file_ext(MULTI_CLIENT_CONFIG_FILE, root_json, JSON_C_TO_STRING_PRETTY);
    return ret;
}

int multi_grp_add_to_config(struct tmcc_nl_service_st *grp)
{
    int ret;
    int fd;

    fd = nf_file_lock(MULTI_CLIENT_CONFIG_FILE);
    VALUE_IS_NEGATIVE_PRINT_AND_RETURN(fd, "lock config file fail", 1);

    ret = __multi_grp_add_to_config(grp);

    nf_file_unlock(fd);

    return ret;
}

int __multi_grp_del_from_config(struct tmcc_nl_service_st *grp)
{
    int i, num, port, ret = 0;
    const char *multi_ip, *server_ip, *new_multi_ip, *new_server_ip;
    struct json_object *root_json, *grp_array, *grp_obj;

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

        new_multi_ip = ip2str(grp->multi_ip);
        new_server_ip = ip2str(grp->server_ip);

        if((strcmp(server_ip, new_server_ip) == 0) && (port == grp->port)){
            n_json_del_element_from_array_by_idx(grp_array, i);
            break;
        }
    }

    if(i == num){  //the group not exist
        printf("group not exist \n");
        ret = -1;
    }

FAIL:
    json_object_to_file_ext(MULTI_CLIENT_CONFIG_FILE, root_json, JSON_C_TO_STRING_PRETTY);
    return ret;
}

int multi_grp_del_from_config(struct tmcc_nl_service_st *grp)
{
    int ret;
    int fd;

    fd = nf_file_lock(MULTI_CLIENT_CONFIG_FILE);
    VALUE_IS_NEGATIVE_PRINT_AND_RETURN(fd, "lock config file fail", 1);

    ret = __multi_grp_del_from_config(grp);

    nf_file_unlock(fd);

    return ret;
}

int __multi_client_clear_config(void)
{
    struct json_object *root_json, *grp_array;

    grp_array = json_get_multi_group_array(&root_json);
    if(grp_array == NULL)
        return -1;

    json_object_object_del(root_json, MULTI_GROUP_ARRAY); 
    json_object_to_file_ext(MULTI_CLIENT_CONFIG_FILE, root_json, JSON_C_TO_STRING_PRETTY);

    return 0;
}

int multi_client_clear_config(void)
{
    int ret;
    int fd;

    fd = nf_file_lock(MULTI_CLIENT_CONFIG_FILE);
    VALUE_IS_NEGATIVE_PRINT_AND_RETURN(fd, "lock config file fail", 1);

    ret = __multi_client_clear_config();

    nf_file_unlock(fd);

    return ret; 
}

