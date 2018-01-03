#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include<sys/file.h>
#include <json.h>
#include <arpa/inet.h>                                                                          
#include <sys/time.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include "grp.h"
#include "multic_json_config.h"

#define JSON_TYPE_ARRAY     json_type_array

int nf_file_lock(char * filename)
{
    int ret; 
    int fd;  

    fd = open(filename, O_RDWR | O_CREAT, S_IRWXU|S_IRWXG);
    if(fd < 0)
        return fd;

    ret = flock(fd, LOCK_EX);
    if(ret < 0){
        close(fd);
        return ret;
    }

    return fd; 
}

int nf_file_unlock(int fd)
{
    int ret; 

    if(fd < 0)
        return fd;

    ret = flock(fd, LOCK_UN);
    close(fd);
    return ret;
}                           

int n_json_del_element_from_array_by_idx( struct json_object * obj, int idx )
{
    int     i;  
    struct array_list *arr;

    if ( !json_object_is_type( obj , JSON_TYPE_ARRAY ) ) 
        return -1; 

    if ( idx >= json_object_array_length( obj ) ) 
        return -1; 

    arr = json_object_get_array( obj );  
    json_object_put( json_object_array_get_idx( obj , idx ) );

    for ( i = idx; i < arr->length - 1; i++ )
        arr->array[i] = arr->array[i+1];
    arr->length--;

    return 0;   
}

int nf_json_config_get_root(char *configfile, const char *key, json_object **root, json_object **root_config)
{
    json_object *json_root;

    if((key == NULL) || (root == NULL) ||(root_config == NULL))
        return -1;

    json_root = json_object_from_file(configfile);
    if(json_root == NULL)
        return -1;

    *root_config = json_object_object_get(json_root, key);
    if(*root_config == NULL)
        return -1;

    *root = json_root;
    return 0;
}

int nf_json_config_init_object(json_object *parent, const char *childkey, json_object **child)
{
    if((parent == NULL) || (childkey == NULL) || (child == NULL))
        return -1;

    *child = json_object_object_get(parent, childkey);
    if(*child == NULL){
        *child = json_object_new_object();
        VALUE_IS_NULL_RETURN(*child, 1);

        json_object_object_add(parent, childkey, *child);
    }

    return 0;
}

int nf_json_config_init_array(json_object *parent, const char *childkey, json_object **child)
{
    if((parent == NULL) || (childkey == NULL) || (child == NULL))
        return -1;

    *child = json_object_object_get(parent, childkey);
    if(*child == NULL){
        *child = json_object_new_array();
        VALUE_IS_NULL_RETURN(*child, 1);

        json_object_object_add(parent, childkey, *child);
    }

    return 0;
}

int nf_json_config_init_int(json_object *parent, const char *childkey, json_object **child, int value)
{
    if((parent == NULL) || (childkey == NULL) || (child == NULL))
        return -1;

    *child = json_object_object_get(parent, childkey);
    if(*child == NULL){
        *child = json_object_new_int(value);
        VALUE_IS_NULL_RETURN(*child, 1);

        json_object_object_add(parent, childkey, *child);
    }

    return 0;
}

int nf_json_config_init_string(json_object *parent, const char *childkey, json_object **child, const char *str)
{
    if((parent == NULL) || (childkey == NULL) || (child == NULL))
        return -1;

    *child = json_object_object_get(parent, childkey);
    if(*child == NULL){
        *child = json_object_new_string(str);
        VALUE_IS_NULL_RETURN(*child, 1);

        json_object_object_add(parent, childkey, *child);
    }

    return 0;
}

int nf_json_config_root_init(json_object **root)
{
    json_object *json_root;

    VALUE_IS_NULL_RETURN(root, 1);

    json_root = *root;

	if(json_root == NULL){
		json_root = json_object_new_object();
		VALUE_IS_NULL_RETURN(json_root, 1);
	}

    *root = json_root;

    return 0;
}
