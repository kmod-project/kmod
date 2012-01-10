SUBDIRS = . libkmod/docs

EXTRA_DIST =
CLEANFILES =
ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}
AM_MAKEFLAGS = --no-print-directory

AM_CPPFLAGS = \
	-include $(top_builddir)/config.h \
	-I$(top_srcdir)/libkmod \
	-DROOTPREFIX=\""$(rootprefix)"\" \
	-DSYSCONFDIR=\""$(sysconfdir)"\" \
	-DLIBEXECDIR=\""$(libexecdir)"\" \
	${zlib_CFLAGS}

AM_CFLAGS = \
	-fvisibility=hidden \
	-ffunction-sections \
	-fdata-sections

AM_LDFLAGS = \
	-Wl,--gc-sections \
	-Wl,--as-needed

SED_PROCESS = \
	$(AM_V_GEN)$(MKDIR_P) $(dir $@) && $(SED) \
	-e 's,@VERSION\@,$(VERSION),g' \
	-e 's,@prefix\@,$(prefix),g' \
	-e 's,@exec_prefix\@,$(exec_prefix),g' \
	-e 's,@libdir\@,$(libdir),g' \
	-e 's,@includedir\@,$(includedir),g' \
	-e 's,@liblzma_CFLAGS\@,${liblzma_CFLAGS},g' \
	-e 's,@liblzma_LIBS\@,${liblzma_LIBS},g' \
	-e 's,@zlib_CFLAGS\@,${zlib_CFLAGS},g' \
	-e 's,@zlib_LIBS\@,${zlib_LIBS},g' \
	< $< > $@ || rm $@

%.pc: %.pc.in Makefile
	$(SED_PROCESS)

LIBKMOD_CURRENT=3
LIBKMOD_REVISION=0
LIBKMOD_AGE=2

noinst_LTLIBRARIES = libkmod/libkmod-util.la
libkmod_libkmod_util_la_SOURCES = libkmod/libkmod-hash.c \
				  libkmod/libkmod-hash.h \
				  libkmod/libkmod-array.c \
				  libkmod/libkmod-array.h \
				  libkmod/libkmod-util.c \
				  libkmod/libkmod-util.h

include_HEADERS = libkmod/libkmod.h
lib_LTLIBRARIES = libkmod/libkmod.la

libkmod_libkmod_la_SOURCES =\
	libkmod/libkmod.h \
	libkmod/libkmod-private.h \
	libkmod/macro.h \
	libkmod/libkmod.c \
	libkmod/libkmod-list.c \
	libkmod/libkmod-config.c \
	libkmod/libkmod-index.c \
	libkmod/libkmod-index.h \
	libkmod/libkmod-module.c \
	libkmod/libkmod-file.c \
	libkmod/libkmod-elf.c

EXTRA_DIST += libkmod/libkmod.sym
EXTRA_DIST += libkmod/COPYING libkmod/README

libkmod_libkmod_la_LDFLAGS = $(AM_LDFLAGS) \
	-version-info $(LIBKMOD_CURRENT):$(LIBKMOD_REVISION):$(LIBKMOD_AGE) \
	-Wl,--version-script=$(top_srcdir)/libkmod/libkmod.sym
libkmod_libkmod_la_DEPENDENCIES = libkmod/libkmod-util.la \
				  ${top_srcdir}/libkmod/libkmod.sym
libkmod_libkmod_la_LIBADD = libkmod/libkmod-util.la \
			    ${liblzma_LIBS} ${zlib_LIBS}

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libkmod/libkmod.pc
EXTRA_DIST += libkmod/libkmod.pc.in
CLEANFILES += libkmod/libkmod.pc

install-exec-hook:
	if test "$(libdir)" != "$(rootlibdir)"; then \
		$(MKDIR_P) $(DESTDIR)$(rootlibdir) && \
		so_img_name=$$(readlink $(DESTDIR)$(libdir)/libkmod.so) && \
		so_img_rel_target_prefix=$$(echo $(libdir) | sed 's,\(^/\|\)[^/][^/]*,..,g') && \
		ln -sf $$so_img_rel_target_prefix$(rootlibdir)/$$so_img_name $(DESTDIR)$(libdir)/libkmod.so && \
		mv $(DESTDIR)$(libdir)/libkmod.so.* $(DESTDIR)$(rootlibdir); \
	fi

uninstall-hook:
	rm -f $(DESTDIR)$(rootlibdir)/libkmod.so*

if BUILD_TOOLS
bin_PROGRAMS = tools/kmod

noinst_SCRIPTS = tools/insmod tools/rmmod tools/lsmod \
		 tools/modprobe tools/modinfo tools/depmod

tools_kmod_SOURCES = tools/kmod.c tools/kmod.h tools/kmod-lsmod.c \
		     tools/kmod-rmmod.c tools/kmod-insmod.c \
		     tools/kmod-modinfo.c tools/kmod-modprobe.c \
		     tools/kmod-depmod.c
tools_kmod_CPPFLAGS = $(AM_CPPFLAGS) -DKMOD_BUNDLE_TOOL=1
tools_kmod_CFLAGS = $(AM_CFLAGS)
tools_kmod_LDADD = libkmod/libkmod-util.la \
		   libkmod/libkmod.la

${noinst_SCRIPTS}: tools/kmod
	$(AM_V_GEN) ($(RM) $@; \
		$(LN_S) $(notdir $<) $@)
endif

check_PROGRAMS = test/test-init test/test-loaded \
		 test/test-insmod test/test-rmmod test/test-rmmod2 \
		 test/test-lookup test/test-path-from-name \
		 test/test-get-dependencies test/test-mod-double-ref \
		 test/test-blacklist test/test-elf test/test-probe \
		 test/test-invalidate-config

TESTS = test/test-init test/test-loaded

test_test_init_LDADD = libkmod/libkmod.la
test_test_loaded_LDADD = libkmod/libkmod.la

test_test_rmmod_LDADD = libkmod/libkmod.la
test_test_rmmod2_LDADD = libkmod/libkmod.la
test_test_insmod_LDADD = libkmod/libkmod.la
test_test_lookup_LDADD = libkmod/libkmod.la
test_test_path_from_name_LDADD = libkmod/libkmod.la
test_test_get_dependencies_LDADD = libkmod/libkmod.la
test_test_mod_double_ref_LDADD = libkmod/libkmod.la
test_test_blacklist_LDADD = libkmod/libkmod.la
test_test_elf_LDADD = libkmod/libkmod.la
test_test_probe_LDADD = libkmod/libkmod.la
test_test_invalidate_config_LDADD = libkmod/libkmod.la

DISTCHECK_CONFIGURE_FLAGS=--enable-gtk-doc
