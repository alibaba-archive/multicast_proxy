KERNELDIR = /lib/modules/`uname -r`/build
INCLUDE := -I$(obj)/../include -I$(obj)/./include
PWD := $(shell pwd)
C_FLAGS += -g $(INCLUDE)
EXTRA_CFLAGS += $(C_FLAGS)
TMCC_MODULE=multi_client

$(TMCC_MODULE)-objs := multic.o multic_nl.o multic_nf.o \
                    multic_grp.o 
obj-m := $(TMCC_MODULE).o

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
install: default
	mkdir -p /usr/local/src/multic/kmod
	mkdir -p /etc/init.d
	cp -f ./multi_client.ko /usr/local/src/multic/kmod/
	cp -f ../scripts/multic /etc/init.d/

clean:
	rm -rf *.o *.ko* *.symvers *.mod.c .*.cmd *.order .rlqos.* .tmp_versions
