KERNELDIR = /lib/modules/`uname -r`/build
INCLUDE := -I$(obj)/../include -I$(obj)/./include
PWD := $(shell pwd)
C_FLAGS += -g $(INCLUDE)
EXTRA_CFLAGS += $(C_FLAGS)
TMCC_MODULE=multi_server

$(TMCC_MODULE)-objs := multis.o multis_nl.o multis_nf.o \
                    multis_grp.o 
obj-m := $(TMCC_MODULE).o

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
install: default
	mkdir -p /etc/init.d
	mkdir -p /usr/local/src/multis/kmod
	cp -f ./multi_server.ko /usr/local/src/multis/kmod/
	cp -f ../scripts/multis /etc/init.d/

clean:
	rm -rf *.o *.ko* *.symvers *.mod.c .*.cmd *.order .rlqos.* .tmp_versions
