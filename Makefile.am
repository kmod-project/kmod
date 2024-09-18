SUBDIRS = . libkmod/docs

if BUILD_MANPAGES
SUBDIRS += man
endif

DISTCLEAN_LOCAL_HOOKS =
EXTRA_DIST =
CLEANFILES = $(BUILT_FILES)
DISTCLEANFILES =
BUILT_FILES =
ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}
AM_MAKEFLAGS = --no-print-directory

GCC_COLORS ?= 'yes'
export GCC_COLORS

# meson bits
EXTRA_DIST += \
	libkmod/docs/meson.build \
	man/meson.build \
	meson.build \
	meson_options.txt \
	testsuite/meson.build \
	scripts/build-scdoc.sh \
	scripts/test-gtkdoc.sh \
	scripts/kmod-symlink.sh

AM_CPPFLAGS = \
	-include $(top_builddir)/config.h \
	-I$(top_srcdir) \
	-DSYSCONFDIR=\""$(sysconfdir)"\" \
	-DDISTCONFDIR=\""$(distconfdir)"\" \
	-DMODULE_DIRECTORY=\""$(module_directory)"\" \
	${zlib_CFLAGS}

AM_CFLAGS = $(OUR_CFLAGS)
AM_LDFLAGS = $(OUR_LDFLAGS)

# Rules for libtool versioning (from https://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html)
# 1. Start with version information of ‘0:0:0’ for each libtool library.
# 2. Update the version information only immediately before a public release of
#    your software. More frequent updates are unnecessary, and only guarantee that
#    the current interface number gets larger faster.
# 3. If the library source code has changed at all since the last update, then
#    increment revision (‘c:r:a’ becomes ‘c:r+1:a’).
# 4. If any interfaces have been added, removed, or changed since the last
#    update, increment current, and set revision to 0.
# 5. If any interfaces have been added since the last public release, then
#    increment age.
# 6. If any interfaces have been removed or changed since the last public
#    release, then set age to 0.
LIBKMOD_CURRENT=7
LIBKMOD_REVISION=0
LIBKMOD_AGE=5

noinst_LTLIBRARIES = shared/libshared.la
shared_libshared_la_SOURCES = \
	shared/array.c \
	shared/array.h \
	shared/hash.c \
	shared/hash.h \
	shared/macro.h \
	shared/missing.h \
	shared/scratchbuf.c \
	shared/scratchbuf.h \
	shared/strbuf.c \
	shared/strbuf.h \
	shared/util.c \
	shared/util.h

include_HEADERS = libkmod/libkmod.h
lib_LTLIBRARIES = libkmod/libkmod.la

libkmod_libkmod_la_SOURCES = \
	libkmod/libkmod-builtin.c \
	libkmod/libkmod.c \
	libkmod/libkmod-config.c \
	libkmod/libkmod-elf.c \
	libkmod/libkmod-file.c \
	libkmod/libkmod.h \
	libkmod/libkmod-index.c \
	libkmod/libkmod-index.h \
	libkmod/libkmod-internal-file.h \
	libkmod/libkmod-internal.h \
	libkmod/libkmod-list.c \
	libkmod/libkmod-module.c \
	libkmod/libkmod-signature.c

if ENABLE_XZ
libkmod_libkmod_la_SOURCES += libkmod/libkmod-file-xz.c
endif

if ENABLE_ZLIB
libkmod_libkmod_la_SOURCES += libkmod/libkmod-file-zlib.c
endif

if ENABLE_ZSTD
libkmod_libkmod_la_SOURCES += libkmod/libkmod-file-zstd.c
endif

EXTRA_DIST += libkmod/libkmod.sym
EXTRA_DIST += libkmod/README \
	libkmod/COPYING testsuite/COPYING tools/COPYING COPYING

libkmod_libkmod_la_LDFLAGS = $(AM_LDFLAGS) \
	-version-info $(LIBKMOD_CURRENT):$(LIBKMOD_REVISION):$(LIBKMOD_AGE) \
	-Wl,--version-script=$(top_srcdir)/libkmod/libkmod.sym
libkmod_libkmod_la_DEPENDENCIES = \
	shared/libshared.la \
	${top_srcdir}/libkmod/libkmod.sym
libkmod_libkmod_la_LIBADD = \
	shared/libshared.la \
	${libzstd_LIBS} ${liblzma_LIBS} ${zlib_LIBS} ${libcrypto_LIBS}

noinst_LTLIBRARIES += libkmod/libkmod-internal.la
libkmod_libkmod_internal_la_SOURCES = $(libkmod_libkmod_la_SOURCES)
libkmod_libkmod_internal_la_LDFLAGS = $(AM_LDFLAGS) \
	-Wl,--version-script=$(top_srcdir)/libkmod/libkmod.sym
libkmod_libkmod_internal_la_DEPENDENCIES  = $(libkmod_libkmod_la_DEPENDENCIES)
libkmod_libkmod_internal_la_LIBADD = $(libkmod_libkmod_la_LIBADD)

pkgconfig_DATA = libkmod/libkmod.pc
noarch_pkgconfig_DATA = tools/kmod.pc

bashcompletiondir=@bashcompletiondir@
dist_bashcompletion_DATA = \
	shell-completion/bash/insmod \
	shell-completion/bash/kmod \
	shell-completion/bash/lsmod \
	shell-completion/bash/rmmod

fishcompletiondir=@fishcompletiondir@
dist_fishcompletion_DATA = \
	shell-completion/fish/insmod.fish \
	shell-completion/fish/lsmod.fish \
	shell-completion/fish/rmmod.fish

zshcompletiondir=@zshcompletiondir@
dist_zshcompletion_DATA = \
	shell-completion/zsh/_insmod \
	shell-completion/zsh/_lsmod \
	shell-completion/zsh/_rmmod

install-exec-hook:
if BUILD_TOOLS
	for tool in insmod lsmod rmmod depmod modprobe modinfo; do \
		$(LN_S) -f kmod $(DESTDIR)$(bindir)/$$tool; \
	done
endif

uninstall-hook:
if BUILD_TOOLS
	for tool in insmod lsmod rmmod depmod modprobe modinfo; do \
		rm -f $(DESTDIR)$(bindir)/$$tool; \
	done
endif

if BUILD_TOOLS
bin_PROGRAMS = tools/kmod

noinst_SCRIPTS = \
	tools/insmod tools/rmmod tools/lsmod \
	tools/modprobe tools/modinfo tools/depmod

tools_kmod_SOURCES = \
	tools/depmod.c \
	tools/insmod.c \
	tools/kmod.c \
	tools/kmod.h \
	tools/log.c \
	tools/log.h \
	tools/lsmod.c \
	tools/modinfo.c \
	tools/modprobe.c \
	tools/rmmod.c \
	tools/static-nodes.c

tools_kmod_LDADD = \
	shared/libshared.la \
	libkmod/libkmod-internal.la

${noinst_SCRIPTS}: tools/kmod
	$(AM_V_GEN) ($(RM) $@; \
		$(LN_S) $(notdir $<) $@)
endif

# ------------------------------------------------------------------------------
# TESTSUITE
# ------------------------------------------------------------------------------

EXTRA_DIST += scripts/setup-modules.sh
EXTRA_DIST += scripts/setup-rootfs.sh

MODULE_PLAYGROUND = testsuite/module-playground
BUILD_MODULES = $(AM_V_GEN) $(top_srcdir)/scripts/setup-modules.sh $(top_srcdir) $(top_builddir) $(MODULE_PLAYGROUND)
ROOTFS = testsuite/rootfs
ROOTFS_PRISTINE = $(top_srcdir)/testsuite/rootfs-pristine
CREATE_ROOTFS = $(AM_V_GEN) $(top_srcdir)/scripts/setup-rootfs.sh $(ROOTFS_PRISTINE) $(ROOTFS) $(MODULE_PLAYGROUND) $(top_builddir)/config.h $(sysconfdir) $(module_directory)

build-module-playground:
	$(BUILD_MODULES)


rootfs: build-module-playground
	$(CREATE_ROOTFS)

.PHONY: rootfs build-playground

$(ROOTFS): $(ROOTFS_PRISTINE)
	$(CREATE_ROOTFS)

TESTSUITE_OVERRIDE_LIBS = \
	testsuite/uname.la testsuite/path.la \
	testsuite/init_module.la \
	testsuite/delete_module.la
TESTSUITE_OVERRIDE_LIBS_LDFLAGS = \
	-avoid-version -module -shared -rpath /nowhere -ldl

check-am: rootfs


EXTRA_DIST += \
	testsuite/module-playground/dummy.pkcs7 \
	testsuite/module-playground/dummy.sha1 \
	testsuite/module-playground/dummy.sha256 \
	testsuite/module-playground/Kbuild \
	testsuite/module-playground/Makefile \
	testsuite/module-playground/Makefile.arch \
	testsuite/module-playground/mod-fake-cciss.c \
	testsuite/module-playground/mod-fake-hpsa.c \
	testsuite/module-playground/mod-fake-scsi-mod.c \
	testsuite/module-playground/mod-foo-a.c \
	testsuite/module-playground/mod-foo-b.c \
	testsuite/module-playground/mod-foo.c \
	testsuite/module-playground/mod-foo-c.c \
	testsuite/module-playground/mod-loop-a.c \
	testsuite/module-playground/mod-loop-b.c \
	testsuite/module-playground/mod-loop-c.c \
	testsuite/module-playground/mod-loop-d.c \
	testsuite/module-playground/mod-loop-e.c \
	testsuite/module-playground/mod-loop-f.c \
	testsuite/module-playground/mod-loop-g.c \
	testsuite/module-playground/mod-loop-h.c \
	testsuite/module-playground/mod-loop-i.c \
	testsuite/module-playground/mod-loop-j.c \
	testsuite/module-playground/mod-loop-k.c \
	testsuite/module-playground/mod-loop.h \
	testsuite/module-playground/mod-simple.c \
	testsuite/module-playground/mod-weakdep.c \
	testsuite/module-playground/README

check_LTLIBRARIES = $(TESTSUITE_OVERRIDE_LIBS)

testsuite_uname_la_LDFLAGS = $(TESTSUITE_OVERRIDE_LIBS_LDFLAGS)
testsuite_path_la_LDFLAGS = $(TESTSUITE_OVERRIDE_LIBS_LDFLAGS)

testsuite_delete_module_la_LDFLAGS = $(TESTSUITE_OVERRIDE_LIBS_LDFLAGS)
testsuite_init_module_la_LDFLAGS = $(TESTSUITE_OVERRIDE_LIBS_LDFLAGS)
testsuite_init_module_la_SOURCES = testsuite/init_module.c \
				   testsuite/stripped-module.h
testsuite_init_module_la_LIBADD = libkmod/libkmod-internal.la

TESTSUITE_CPPFLAGS = $(AM_CPPFLAGS) \
	-DTESTSUITE_ROOTFS=\"$(abs_top_builddir)/$(ROOTFS)/\" \
	-DTOOLS_DIR=\"$(abs_top_builddir)/tools\" \
	-DOVERRIDE_LIBDIR=\"$(abs_top_builddir)/testsuite/.libs/\"
TESTSUITE_LDADD = \
	testsuite/libtestsuite.la libkmod/libkmod-internal.la \
	shared/libshared.la

check_LTLIBRARIES += testsuite/libtestsuite.la
testsuite_libtestsuite_la_SOURCES = \
	testsuite/testsuite.c testsuite/testsuite.h
testsuite_libtestsuite_la_DEPENDENCIES = \
	$(ROOTFS) $(TESTSUITE_OVERRIDE_LIBS)
testsuite_libtestsuite_la_CPPFLAGS = $(TESTSUITE_CPPFLAGS)
testsuite_libtestsuite_la_LIBADD = -lrt

TESTSUITE = \
	testsuite/test-array \
	testsuite/test-blacklist \
	testsuite/test-dependencies \
	testsuite/test-depmod \
	testsuite/test-hash \
	testsuite/test-init \
	testsuite/test-initstate \
	testsuite/test-list \
	testsuite/test-loaded \
	testsuite/test-modinfo \
	testsuite/test-modprobe \
	testsuite/test-new-module \
	testsuite/test-scratchbuf \
	testsuite/test-strbuf \
	testsuite/test-testsuite \
	testsuite/test-util \
	testsuite/test-weakdep

check_PROGRAMS = $(TESTSUITE)
TESTS = $(TESTSUITE)

testsuite_test_testsuite_LDADD = \
	testsuite/libtestsuite.la shared/libshared.la
testsuite_test_testsuite_CPPFLAGS = $(TESTSUITE_CPPFLAGS)

testsuite_test_hash_LDADD = $(TESTSUITE_LDADD)
testsuite_test_hash_CPPFLAGS = $(TESTSUITE_CPPFLAGS)

testsuite_test_array_LDADD = $(TESTSUITE_LDADD)
testsuite_test_array_CPPFLAGS = $(TESTSUITE_CPPFLAGS)

testsuite_test_scratchbuf_LDADD = $(TESTSUITE_LDADD)
testsuite_test_scratchbuf_CPPFLAGS = $(TESTSUITE_CPPFLAGS)

testsuite_test_strbuf_LDADD = $(TESTSUITE_LDADD)
testsuite_test_strbuf_CPPFLAGS = $(TESTSUITE_CPPFLAGS)

testsuite_test_init_LDADD = $(TESTSUITE_LDADD)
testsuite_test_init_CPPFLAGS = $(TESTSUITE_CPPFLAGS)
testsuite_test_initstate_LDADD = $(TESTSUITE_LDADD)
testsuite_test_initstate_CPPFLAGS = $(TESTSUITE_CPPFLAGS)
testsuite_test_loaded_LDADD = $(TESTSUITE_LDADD)
testsuite_test_loaded_CPPFLAGS = $(TESTSUITE_CPPFLAGS)
testsuite_test_modinfo_LDADD = $(TESTSUITE_LDADD)
testsuite_test_modinfo_CPPFLAGS = $(TESTSUITE_CPPFLAGS)
testsuite_test_util_LDADD = $(TESTSUITE_LDADD)
testsuite_test_util_CPPFLAGS = $(TESTSUITE_CPPFLAGS)
testsuite_test_new_module_LDADD = $(TESTSUITE_LDADD)
testsuite_test_new_module_CPPFLAGS = $(TESTSUITE_CPPFLAGS)
testsuite_test_modprobe_LDADD = $(TESTSUITE_LDADD)
testsuite_test_modprobe_CPPFLAGS = $(TESTSUITE_CPPFLAGS)
testsuite_test_blacklist_LDADD = $(TESTSUITE_LDADD)
testsuite_test_blacklist_CPPFLAGS = $(TESTSUITE_CPPFLAGS)
testsuite_test_dependencies_LDADD = $(TESTSUITE_LDADD)
testsuite_test_dependencies_CPPFLAGS = $(TESTSUITE_CPPFLAGS)
testsuite_test_depmod_LDADD = $(TESTSUITE_LDADD)
testsuite_test_depmod_CPPFLAGS = $(TESTSUITE_CPPFLAGS)
testsuite_test_list_LDADD = $(TESTSUITE_LDADD)
testsuite_test_list_CPPFLAGS = $(TESTSUITE_CPPFLAGS)
testsuite_test_weakdep_LDADD = $(TESTSUITE_LDADD)
testsuite_test_weakdep_CPPFLAGS = $(TESTSUITE_CPPFLAGS)

testsuite-distclean:
	$(RM) -r $(ROOTFS)
	$(RM) testsuite/stamp-rootfs
	$(MAKE) -C $(MODULE_PLAYGROUND) clean
	if test "$(top_srcdir)" != "$(top_builddir)"; then \
		$(RM) -rf testsuite/module-playground; \
	fi

DISTCLEAN_LOCAL_HOOKS += testsuite-distclean
EXTRA_DIST += testsuite/rootfs-pristine

AM_DISTCHECK_CONFIGURE_FLAGS=--enable-gtk-doc --sysconfdir=/etc \
	--with-zlib --with-zstd --with-xz --with-openssl \
	--with-bashcompletiondir=$$dc_install_base/$(bashcompletiondir) \
	--with-fishcompletiondir=$$dc_install_base/$(fishcompletiondir) \
	--with-zshcompletiondir=$$dc_install_base/$(zshcompletiondir)

distclean-local: $(DISTCLEAN_LOCAL_HOOKS)

buildtest-TESTS:
	$(MAKE) $(AM_MAKEFLAGS) $(check_LTLIBRARIES) $(check_PROGRAMS)

# ------------------------------------------------------------------------------
# coverage
# ------------------------------------------------------------------------------

# .PHONY so it always rebuilds it
.PHONY: coverage lcov-run lcov-report coverage-sync

# run lcov from scratch, always
coverage: all
	$(MAKE) lcov-run
	$(MAKE) lcov-report

coverage_dir = coverage
coverage_opts = --base-directory $(srcdir) --directory $(builddir) \
		--rc 'geninfo_adjust_src_path=$(abspath $(srcdir))=>$(abspath $(builddir))'

if ENABLE_COVERAGE
# reset run coverage tests
lcov-run:
	@rm -rf $(coverage_dir)
	lcov $(coverage_opts) --zerocounters
	-$(MAKE) check

# generate report based on current coverage data
lcov-report:
	$(MKDIR_P) $(coverage_dir)
	lcov $(coverage_opts) --capture --no-external --ignore-errors graph \
		| sed 's|$(abspath $(builddir))|$(abspath $(srcdir))|' > $(coverage_dir)/.lcov.info
	lcov --remove $(coverage_dir)/.lcov.info --output-file $(coverage_dir)/.lcov-clean.info 'test-*'
	genhtml -t "kmod test coverage" -o $(coverage_dir) $(coverage_dir)/.lcov-clean.info
	@echo "Coverage report generated in $(abs_builddir)/$(coverage_dir)/index.html"

else

lcov-run lcov-report:
	echo "Need to reconfigure with --enable-coverage"

endif

# ------------------------------------------------------------------------------
# coverity
# ------------------------------------------------------------------------------

kmod-coverity-%.tar.xz:
	rm -rf $< cov-int
	./autogen.sh c --disable-manpages
	make clean
	cov-build --dir cov-int make -j 4
	tar caf $@ cov-int

coverity-tar: kmod-coverity-$(shell git describe  2>/dev/null).tar.xz

coverity-sync: kmod-coverity-$(shell git describe 2>/dev/null).tar.xz
	@echo "uploading coverity tarball"
	@curl --form token=$(COVERITY_KMOD_TOKEN) \
		--form email=lucas.de.marchi@gmail.com \
		--form file=@$< \
		--form version="$(shell git describe)" \
		--form description="" \
		https://scan.coverity.com/builds?project=kmod

coverity-clean:
	rm -rf kmod-coverity-*.tar.xz cov-int

# ------------------------------------------------------------------------------
# custom release helpers
# ------------------------------------------------------------------------------

git-release:
	head -1 NEWS | grep -q "kmod $(VERSION)"
	git commit -a -s -m "kmod $(VERSION)"
	git tag -m "kmod $(VERSION)" -s v$(VERSION)
	git gc --prune=0

kmod-$(VERSION).tar.xz:
	make distcheck

kmod-$(VERSION).tar.sign:
	xz -d -c kmod-$(VERSION).tar.xz | gpg --armor --detach-sign --output kmod-$(VERSION).tar.sign

tar: kmod-$(VERSION).tar.xz kmod-$(VERSION).tar.sign

tar-sync: kmod-$(VERSION).tar.xz kmod-$(VERSION).tar.sign
	kup put kmod-$(VERSION).tar.xz  kmod-$(VERSION).tar.sign /pub/linux/utils/kernel/kmod/
