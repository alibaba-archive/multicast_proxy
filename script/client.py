#!/usr/bin/env python
import sys
import socket
import struct

multi_ip = '239.254.1.2'
multi_port = 65535

if __name__ == '__main__':
    if len(sys.argv) == 3:
        multi_ip = sys.argv[1]
        multi_port = sys.argv[2]
    else:
        print 'Usage: python %s multi_ip multi_port. Now default conf is used.' % sys.argv[0]

    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind(('', multi_port))
    mreq = struct.pack("=4sl", socket.inet_aton(multi_ip), socket.INADDR_ANY)
    s.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
    while True:
        print "recvied from server" + s.recv(1024)
