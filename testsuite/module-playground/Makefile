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
# 1) mod-loop-a  ->  mod-loop-b -> mod-loop-c -> mod-loop-a
#     |-> mod-loop-f    |-> mod-loop-f
#     \-> mod-loop-g    \-> mod-loop-g
# 2) mod-loop-d -> mod-loop-e -> mod-loop-d
# 3.1) mod-loop-h -> mod-loop-i -> mod-loop-j -> mod-loop-h
# 3.2) mod-loop-h -> mod-loop-i -> mod-loop-j -> mod-loop-k -> mod-loop-h

obj-m += mod-loop-a.o
obj-m += mod-loop-b.o
obj-m += mod-loop-c.o
obj-m += mod-loop-d.o
obj-m += mod-loop-e.o
obj-m += mod-loop-f.o
obj-m += mod-loop-g.o
obj-m += mod-loop-h.o
obj-m += mod-loop-i.o
obj-m += mod-loop-j.o
obj-m += mod-loop-k.o

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
ifeq ($(FAKE_BUILD),)
    FAKE_BUILD=0
endif

ARCH_SPECIFIC_MODULES := mod-simple-x86_64.ko mod-simple-i386.ko mod-simple-sparc64.ko
MY_MODULES := $(filter-out $(ARCH_SPECIFIC_MODULES),$(wildcard *.ko))

default: modules arch-modules

mod-simple-%.ko: mod-simple-%.c Makefile.arch
	$(eval arch=$(patsubst mod-simple-%.ko,%,$@))
	$(MAKE) KDIR=$(KDIR_$(arch)) ARCH=$(arch) CROSS_COMPILE=$(CROSS_COMPILE_$(arch)) -f Makefile.arch

ifeq ($(FAKE_BUILD),0)
modules:
	$(MAKE) -C $(KDIR) M=$$PWD
else
modules:
	@echo "  CP       cache/*.ko"
	@cp cache/*.ko .
endif

arch-modules: $(ARCH_SPECIFIC_MODULES)

clean:
	test -z "$(MY_MODULES)" || rm -rf $(MY_MODULES) || true
	rm -rf .tmp_versions || true
	rm -rf .*.cmd *.mod.c || true
	rm -rf *.o || true
	rm -rf Module.symvers modules.order

endif
