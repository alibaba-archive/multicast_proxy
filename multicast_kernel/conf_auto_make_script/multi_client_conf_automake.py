#!/bin/python
# -*- coding: utf-8 -*-
import json, os, commands, sys

FILENAME="iplist"
### Format
##Line1 multi_ip multi_port
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
        print "%s does not exist" %filename
        sys.exit(-1)

    local_vmip = get_local_ip()

    i = 0
    multi_ip = ""
    multi_port = ""

    with open(filename) as f:
        lines_info = f.readlines()
        for line in lines_info:
            if '#' in line:
                continue

            if i == 0:
                multi_ip, multi_port = line.strip('\n').split()
                i = 1
            else:
                if local_vmip in line:
                    continue
                server_info = {}
                server_info["server_ip"] = line.strip('\n')
                server_info["group_ip"] = multi_ip
                server_info["client_port"] = multi_port

                json_cfg["multi_server_info_array"].append(server_info)


json_cfg = {
    "multi_server_info_array":[
    ]
}


if __name__ == "__main__":
    conf_init(FILENAME)
    print json_cfg
    with open("multi_client_running_config", "w") as dump_f:
        json.dump(json_cfg, dump_f)

    cmd = "cp multi_client_running_config /usr/local/etc/multi_client_running_config"
    status, value = commands.getstatusoutput(cmd)
    if (status != 0):
        print "client config auto make error"

    print "client config auto make ok, please restart multis service"
