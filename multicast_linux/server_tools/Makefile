CC = gcc -Wall
RM = rm -rf

SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c, %.o, $(SRCS))

CFLAGS =

LIBS = -L/usr/local/lib -ljson-c -lpcap -lpthread

INC = -I/usr/local/include/json-c -I/usr/local/include/pcap

TARGET := server_reload_list


all: server_reload_list

server_reload_list:
	$(CC) -c $(SRCS) $(INC)
	$(CC) -o $(TARGET) $(CFLAGS) $(OBJS) $(LIBS)  

clean:
	$(RM) $(OBJS) $(TARGET)
