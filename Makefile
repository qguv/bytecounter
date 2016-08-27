# Kernel module Makefile
# adapted from http://www.tldp.org/LDP/lkmpg/2.6/html/x181.html

obj-m += bytecounter.o

KERNEL_TREE ?= /lib/modules/$(shell uname -r)/build

.PHONY: all clean

all:
	make -C $(KERNEL_TREE) M=$(PWD) modules

clean:
	make -C $(KERNEL_TREE) M=$(PWD) clean
