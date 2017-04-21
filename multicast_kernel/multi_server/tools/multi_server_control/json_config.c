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
#include "multis_json_config.h"
#include "tmcc_nl.h"

static const char *ip2str(uint32_t ip)  
{  
    struct in_addr a;  
    a.s_addr = htonl(ip);  
    return inet_ntoa(a);  
}  

struct json_object *json_get_multi_group_array(struct json_object **root)
{
    struct json_object *root_json, *grp_array;

    root_json = json_object_from_file(MULTI_SERVER_CONFIG_FILE);
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
    int i, j, num, ret = 0;
    const char *multi_ip, *new_multi_ip, *new_mem_ip, *mem_ip;
    struct json_object *root_json, *grp_array, *grp_obj, *mem_array;

    grp_array = json_get_multi_group_array(&root_json);
    if(grp_array == NULL)
        return -1;
        
    new_multi_ip = ip2str(grp->multi_ip);
    num = json_object_array_length(grp_array);
    for(i=0; i<num; i++){
        grp_obj = json_object_array_get_idx(grp_array, i);
        VALUE_IS_NULL_PRINT_AND_GOTO_FAIL(grp_obj, "json format wrong", 1);

        multi_ip = json_object_get_string(json_object_object_get(grp_obj, MULTI_GROUP_IP));
        VALUE_IS_NULL_PRINT_AND_GOTO_FAIL(multi_ip, "json format wrong", 1);

        if(strcmp(new_multi_ip, multi_ip) == 0)
            break;
    }

    if(i == num){  //the group not exist
        if(grp->action == MULTI_OPT_QUIT){
            printf("multigroup not exist\n");
            ret = -1;
            goto FAIL;
        }

        if(num + 1 > MULTI_GRP_MAX){
            printf("error: you can only add %d multicast groups\n", MULTI_GRP_MAX);
            ret = -1;
            goto FAIL;
        }

        if(grp->ip_num > MULTI_VM_MAX){
            printf("error: you can only add %d multicast member\n", MULTI_VM_MAX);
            ret = -1;
            goto FAIL;
        }

        grp_obj = json_object_new_object();
        VALUE_IS_NULL_PRINT_AND_GOTO_FAIL(grp_obj, "json_object_new_object fail", -1);

        json_object_object_add(grp_obj, MULTI_GROUP_IP, json_object_new_string(new_multi_ip));
        mem_array = json_object_new_array();
        for(j =0; j < grp->ip_num; j++){
            new_mem_ip = ip2str(grp->ip_list[j]);
            json_object_array_add(mem_array, json_object_new_string(new_mem_ip));
        }

        json_object_object_add(grp_obj, MULTI_MEMBER_ARRAY, mem_array);
        json_object_array_add(grp_array, grp_obj);
    }
    else{
        mem_array = json_object_object_get(grp_obj, MULTI_MEMBER_ARRAY);

        num = json_object_array_length(mem_array);
        if((grp->action == MULTI_OPT_JOIN) && (grp->ip_num + num > MULTI_VM_MAX)){
            printf("error: you can only add %d multicast member\n", MULTI_VM_MAX);
            ret = -1;
            goto FAIL;
        }
        
        for(i = 0; i<grp->ip_num; i++){
            new_mem_ip = ip2str(grp->ip_list[i]);
            num = json_object_array_length(mem_array);
            for(j = 0; j< num; j++){
                mem_ip = json_object_get_string(json_object_array_get_idx(mem_array, j));
                if(strcmp(mem_ip, new_mem_ip) == 0)
                    break;
            }

            if(grp->action == MULTI_OPT_JOIN){
                if(j == num){
                    json_object_array_add(mem_array, json_object_new_string(new_mem_ip));
                } 
            }
            else if(grp->action == MULTI_OPT_QUIT){
                if(j < num){
                    n_json_del_element_from_array_by_idx(mem_array, j);
                }
            }
        }
    }


FAIL:
    json_object_to_file_ext(MULTI_SERVER_CONFIG_FILE, root_json, JSON_C_TO_STRING_PRETTY);
    return ret;
}

int multi_grp_add_to_config(struct tmcc_nl_service_st *grp)
{
    int ret;
    int fd;

    fd = nf_file_lock(MULTI_SERVER_CONFIG_FILE);
    VALUE_IS_NEGATIVE_PRINT_AND_RETURN(fd, "lock config file fail", 1);

    ret = __multi_grp_add_to_config(grp);

    nf_file_unlock(fd);

    return ret;
}

int __multi_grp_del_from_config(struct tmcc_nl_service_st *grp)
{
    int i, num, ret = 0;
    const char *multi_ip, *new_multi_ip;
    struct json_object *root_json, *grp_array, *grp_obj;

    grp_array = json_get_multi_group_array(&root_json);
    if(grp_array == NULL)
        return -1;

    new_multi_ip = ip2str(grp->multi_ip);
    num = json_object_array_length(grp_array);
    for(i=0; i<num; i++){
        grp_obj = json_object_array_get_idx(grp_array, i);
        VALUE_IS_NULL_PRINT_AND_GOTO_FAIL(grp_obj, "json format wrong", 1);

        multi_ip = json_object_get_string(json_object_object_get(grp_obj, MULTI_GROUP_IP));
        VALUE_IS_NULL_PRINT_AND_GOTO_FAIL(multi_ip, "json format wrong", 1);

        if(strcmp(multi_ip, new_multi_ip) == 0){
            n_json_del_element_from_array_by_idx(grp_array, i);
            break;
        }
    }

    if(i == num){  //the group not exist
        printf("group not exist \n");
        ret = -1;
    }

FAIL:
    json_object_to_file_ext(MULTI_SERVER_CONFIG_FILE, root_json, JSON_C_TO_STRING_PRETTY);
    return ret;
}

int multi_grp_del_from_config(struct tmcc_nl_service_st *grp)
{
    int ret;
    int fd;

    fd = nf_file_lock(MULTI_SERVER_CONFIG_FILE);
    VALUE_IS_NEGATIVE_PRINT_AND_RETURN(fd, "lock config file fail", 1);

    ret = __multi_grp_del_from_config(grp);

    nf_file_unlock(fd);

    return ret;
}

void __multi_server_config_clear(void)
{
    struct json_object *root_json, *grp_array;

    grp_array = json_get_multi_group_array(&root_json);
    if(grp_array == NULL)
        return;

    json_object_object_del(root_json, MULTI_GROUP_ARRAY); 
    json_object_to_file_ext(MULTI_SERVER_CONFIG_FILE, root_json, JSON_C_TO_STRING_PRETTY);
    return;
}


int multi_server_config_clear(void)
{
    int fd;

    fd = nf_file_lock(MULTI_SERVER_CONFIG_FILE);
    VALUE_IS_NEGATIVE_PRINT_AND_RETURN(fd, "lock config file fail", 1);

    __multi_server_config_clear();

    nf_file_unlock(fd);
    return 0;
}
