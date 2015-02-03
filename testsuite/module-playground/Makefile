ifneq ($(KERNELRELEASE),)
# kbuild part of makefile
obj-m := mod-simple.o

# mod-foo depends on foo-x, and foo-x modules don't depend
# on anyone
obj-m += mod-foo-a.o
obj-m += mod-foo-b.o
obj-m += mod-foo-c.o
obj-m += mod-foo.o

else
# normal makefile
KDIR ?= /lib/modules/`uname -r`/build
KVER ?= `uname -r`

default:
	$(MAKE) -C $(KDIR) M=$$PWD

clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean

endif
