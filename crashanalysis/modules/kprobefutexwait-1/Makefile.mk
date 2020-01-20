obj-m +=kprobefutexwait.o
KDIR= /lib/modules/$(shell uname -r)/build
all:
	$(MAKE) -C $(KDIR) SUBDIRS=$(CURDIR) modules
clean:
	rm -rf *.o *.ko *.mod.* .c* .t* *.symvers *.order
