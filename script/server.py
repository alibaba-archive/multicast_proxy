#!/usr/bin/env python
import sys
import socket
import struct
import time

multi_ip = '239.254.1.2'
multi_port = 65535
eth_ip = '127.0.0.1'

def main(multi_ip, multi_port, eth_ip):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 32)
    #sock.bind((eth_ip, multi_port))
    i=0
    while i < 15:
        i = i + 1
        sock.sendto(time.strftime('(%Y-%m-%d %H:%M:%S)' + '\n'), (multi_ip, multi_port))
        print "send %s pkt %s %s" %(i, multi_ip, eth_ip)
        time.sleep(1)

if __name__ == '__main__':
    if len(sys.argv) == 4:
        multi_ip = sys.argv[1]
        multi_port = int(sys.argv[2])
        eth_ip = sys.argv[3]
    else:
        print 'Usage: python %s multi_ip multi_port eth_ip. Now default conf is used.' %sys.argv[0]

    main(multi_ip, multi_port, eth_ip)

