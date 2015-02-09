ifneq ($(KERNELRELEASE),)
# kbuild part of makefile
obj-m := mod-simple.o

# mod-foo depends on foo-x, and foo-x modules don't depend
# on anyone
obj-m += mod-foo-a.o
obj-m += mod-foo-b.o
obj-m += mod-foo-c.o
obj-m += mod-foo.o

# mod-loop: create loops in dependencies:
# 1) mod-loop-a -> mod-loop-b -> mod-loop-c -> mod-loop-a
# 2) mod-loop-d -> mod-loop-e -> mod-loop-d
obj-m += mod-loop-a.o
obj-m += mod-loop-b.o
obj-m += mod-loop-c.o
obj-m += mod-loop-d.o
obj-m += mod-loop-e.o

else
# normal makefile
KDIR ?= /lib/modules/`uname -r`/build
KVER ?= `uname -r`

default:
	$(MAKE) -C $(KDIR) M=$$PWD

clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean

endif
