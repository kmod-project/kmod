ifneq ($(KERNELRELEASE),)
# kbuild part of makefile
obj-m := mod-simple.o
else
# normal makefile
KDIR ?= /lib/modules/`uname -r`/build
KVER ?= `uname -r`

default:
	$(MAKE) -C $(KDIR) M=$$PWD

clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean

endif
