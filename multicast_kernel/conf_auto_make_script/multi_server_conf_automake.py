#!/bin/python
# -*- coding: utf-8 -*-
import json, os, commands, sys

FILENAME="iplist"
### Format iplist
##Line1 multi_ip, multi_port
##Line2-n normal_ip

def get_local_ip():
    cmd = "ip addr show dev eth0 | grep inet | awk -F' ' '{print $2}' | awk -F'/' '{print $1}'"
    status, value = commands.getstatusoutput(cmd)
    if status == 0:
        return value

    return 0


def conf_init(filename):
    ret = os.path.exists(filename)
    if ret is False:
        print "file %s does not exist" %filename
        sys.exit(-1)

    local_vmip = get_local_ip()

    i = 0
    with open(filename) as f:
        lines_info = f.readlines()
        for line in lines_info:
            if '#' in line:
                continue

            if len(line) < 7:
                continue

            if i == 0:
                json_cfg["multi_group_array"][0]["group_ip"] = line.strip('\n').split()[0]
                i = 1
            else:
                if local_vmip in line:
                    continue
                json_cfg["multi_group_array"][0]["member_array"].append(line.strip('\n'))


json_cfg = { 
    "multi_group_array" : [
        {
            "group_ip" : "",
            "member_array" : []
        }    
    ]
}


if __name__ == "__main__":
    conf_init(FILENAME)
    print json_cfg
    with open("multi_server_running_config", "w") as dump_f:
        json.dump(json_cfg, dump_f)

    cmd = "cp multi_server_running_config /usr/local/etc/multi_server_running_config"
    status, value = commands.getstatusoutput(cmd)
    if (status != 0):
        print "server config auto make error"

    print "server config auto make ok, please restart multis service"
