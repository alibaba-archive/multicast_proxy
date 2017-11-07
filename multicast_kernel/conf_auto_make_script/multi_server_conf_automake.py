#!/bin/python
# -*- coding: utf-8 -*-
import json, os, commands, sys

IPLIST_FILE="iplist"
CONF_FILE="/usr/local/etc/multi_server_running_config"
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

    get_info_from_conf(filename)


def get_info_from_conf(filename):
    global json_cfg
    i = 0
    server_info = {
        "group_ip" : "",
        "member_array" : []
    }
    local_vmip = get_local_ip()

    ret = os.path.exists(CONF_FILE)
    if ret is True:
        with open(CONF_FILE) as f:
            json_cfg = json.load(f)

    with open(filename) as f:
        lines_info = f.readlines()
        for line in lines_info:
            if '#' in line:
                continue

            if len(line) < 7:
                continue

            if i == 0:
                server_info["group_ip"] = line.strip('\n').split()[0]
                i = 1
            else:
                if local_vmip in line:
                    continue
                server_info["member_array"].append(line.strip('\n'))

    if server_info not in json_cfg["multi_group_array"]:
        json_cfg["multi_group_array"].append(server_info)


json_cfg = { 
    "multi_group_array" : [
    ]
}


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print "Usage: python %s filename" %(sys.argv[0])
        sys.exit(1)

    IPLIST_FILE=sys.argv[1]
    conf_init(IPLIST_FILE)

    with open("multi_server_running_config", "w") as dump_f:
        json.dump(json_cfg, dump_f)

    cmd = "mv multi_server_running_config %s" %CONF_FILE
    status, value = commands.getstatusoutput(cmd)
    if (status != 0):
        print "server config auto make error"

    print "server config auto make ok, please run <service multis restart>"
