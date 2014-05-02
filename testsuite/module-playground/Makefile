ifneq ($(KERNELRELEASE),)
# kbuild part of makefile
obj-m := moduleA.o moduleB.o moduleC.o
else
# normal makefile
KDIR ?= /lib/modules/`uname -r`/build
KVER ?= `uname -r`
TESTDIR_BASE := /tmp/test-kmod
TESTDIR := $(TESTDIR_BASE)/lib/modules/$(KVER)/

default:
	$(MAKE) -C $(KDIR) M=$$PWD

install: default
	mkdir -p $(TESTDIR)/kernel/
	cp *.ko $(TESTDIR)/kernel/
	cp modules.order $(TESTDIR)
	depmod -a -b $(TESTDIR_BASE) $(KVER)

clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean

endif
