#!/bin/sh
#****************************************************************#
# ScriptName: test.sh
# Author: $SHTERM_REAL_USER@alibaba-inc.com
# Create Date: 2014-10-28 13:14
# Modify Author: $SHTERM_REAL_USER@alibaba-inc.com
# Modify Date: 2014-10-29 14:26
# Function: 
#***************************************************************#


./multi_client_app -A -i 10.107.5.114 -p 1234 -m 235.24.24.24
./multi_client_app -A -i 172.16.1.1 -p 1235 -m 225.24.24.24
./multi_client_app -A -i 172.16.1.2 -p 1235 -m 226.24.24.24
./multi_client_app -A -i 172.16.1.3 -p 1235 -m 227.24.24.24
./multi_client_app -A -i 172.16.1.4 -p 1235 -m 228.24.24.24
./multi_client_app -A -i 172.16.1.5 -p 1235 -m 229.24.24.24
./multi_client_app -A -i 172.16.1.6 -p 1235 -m 230.24.24.24
./multi_client_app -A -i 172.16.1.7 -p 1235 -m 231.24.24.24
./multi_client_app -A -i 172.16.1.8 -p 1235 -m 232.24.24.24
./multi_client_app -A -i 172.16.1.9 -p 1235 -m 233.24.24.24
./multi_client_app -A -i 172.16.1.10 -p 1235 -m 234.24.24.24
