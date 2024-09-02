KDIR ?= $(MODULE_DIRECTORY)/$$(uname -r)/build

ARCH_SPECIFIC_MODULES := mod-simple-x86_64.ko mod-simple-i386.ko mod-simple-sparc64.ko

default: modules

mod-simple-%.ko: mod-simple-%.c Makefile.arch
	$(eval arch=$(patsubst mod-simple-%.ko,%,$@))
	$(MAKE) KDIR=$(KDIR_$(arch)) ARCH=$(arch) CROSS_COMPILE=$(CROSS_COMPILE_$(arch)) -f Makefile.arch

modules:
	$(MAKE) -C $(KDIR) M=$$PWD modules

arch-modules: $(ARCH_SPECIFIC_MODULES)

clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean
