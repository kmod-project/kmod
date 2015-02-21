ifneq ($(KERNELRELEASE),)
# kbuild part of makefile

ifneq ($(KMOD_TESTSUITE_ARCH_BUILD),1)
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

# mod-fake-*: fake the respective modules in kernel with these aliases. Aliases
# list was taken from 3.5.4
obj-m += mod-fake-hpsa.o
obj-m += mod-fake-scsi-mod.o
obj-m += mod-fake-cciss.o

else
# only build ARCH-specific module
ifeq ($(ARCH),)
    $(error ARCH must be set to a valid architecture)
endif
obj-m := mod-simple-$(ARCH).o
endif

else
# normal makefile
KDIR ?= /lib/modules/`uname -r`/build
KVER ?= `uname -r`

ARCH_SPECIFIC_MODULES := mod-simple-x86_64.ko mod-simple-i386.ko mod-simple-sparc64.ko

default: modules arch-modules

mod-simple-%.ko: mod-simple-%.c Makefile.arch
	$(eval arch=$(patsubst mod-simple-%.ko,%,$@))
	$(MAKE) KDIR=$(KDIR_$(arch)) ARCH=$(arch) CROSS_COMPILE=$(CROSS_COMPILE_$(arch)) -f Makefile.arch

modules:
	$(MAKE) -C $(KDIR) M=$$PWD

arch-modules: $(ARCH_SPECIFIC_MODULES)

clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean
	$(MAKE) KDIR=$(KDIR_x86_64) ARCH=x86_64 CROSS_COMPILE=$(CROSS_COMPILE_x86_64) -f Makefile.arch clean
	$(MAKE) KDIR=$(KDIR_i386) ARCH=i386 CROSS_COMPILE=$(CROSS_COMPILE_i386) -f Makefile.arch clean
	$(MAKE) KDIR=$(KDIR_sparc64) ARCH=sparc64 CROSS_COMPILE=$(CROSS_COMPILE_sparc64) -f Makefile.arch clean

endif
