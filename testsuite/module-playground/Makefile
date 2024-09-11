# When KDIR is not manually set, use the MODULE_DIRECTORY for the pre-existent
# kmod, not the one being built, to figure out how to get to the kernel
# sources/headers for building our own dummy kernel modules.  The final
# location of the modules may change later by scripts/setup-rootfs.sh that
# assembles a per-test-rootfs.
ifndef KDIR
  MODULE_DIRECTORY := $(shell pkg-config --variable module_directory kmod)
  ifeq ($(MODULE_DIRECTORY),)
    MODULE_DIRECTORY := /lib/modules
  endif
  KDIR := $(MODULE_DIRECTORY)/$$(uname -r)/build
endif

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
