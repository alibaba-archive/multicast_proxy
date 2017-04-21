CC=gcc
CFLAGS=-Wall -g -I../../include -I../../thirdpart/json-c/ -I../../thirdpart/libcurl/
LD_FLAGS=-L../../thirdpart/json-c -ljson-c
#LD_FLAGS+=-L../../thirdpart/libcurl -lcurl -lrt
PROGS=multic_admin
OBJS=app_control.o comm_kernel.o json_config.o base_config.o
%.o:%.c %.h
$(PROGS):$(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LD_FLAGS) -o $(PROGS) 

all:multic_admin
install:multic_admin
	mkdir -p /usr/local/sbin
	cp -f ./multic_admin /usr/local/sbin/


.PHONY:clean

clean:
	rm -fr *.o $(PROGS) 
