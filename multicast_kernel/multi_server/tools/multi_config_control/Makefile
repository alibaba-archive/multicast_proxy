CC=gcc
CFLAGS=-Wall -g -I../../include -I../../thirdpart/json-c/ -I../../thirdpart/libcurl/
LD_FLAGS=-L../../thirdpart/json-c -ljson-c
#LD_FLAGS+=-L../../thirdpart/libcurl -lcurl -lrt
PROGS=multis_config_admin
OBJS=app_control.o json_config.o base_config.o
%.o:%.c %.h
$(PROGS):$(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LD_FLAGS) -o $(PROGS) 

all:multis_config_admin
install:multis_config_admin
	mkdir -p /usr/local/sbin
	cp -f ./multis_config_admin /usr/local/sbin/

.PHONY:clean

clean:
	rm -fr *.o $(PROGS) 
