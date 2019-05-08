CC=gcc
CFLAGS=-Wall -g -I../../include -I../../thirdpart/json-c/
LD_FLAGS=-L../../thirdpart/json-c -ljson-c
PROGS=multis_admin
OBJS=app_control.o comm_kernel.o json_config.o base_config.o
%.o:%.c %.h
$(PROGS):$(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LD_FLAGS) -o $(PROGS) 
all:multis_admin
install:multis_admin
	mkdir -p /usr/local/sbin
	cp -f ./multis_admin /usr/local/sbin/
.PHONY:clean
clean:
	rm -fr *.o $(PROGS) 
