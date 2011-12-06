EXTRA_DIST =
CLEANFILES =
ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}
AM_MAKEFLAGS = --no-print-directory

AM_CPPFLAGS = \
	-include $(top_builddir)/config.h \
	-I$(top_srcdir)/libkmod \
	-DSYSCONFDIR=\""$(sysconfdir)"\" \
	-DLIBEXECDIR=\""$(libexecdir)"\"

AM_CFLAGS = \
	-fvisibility=hidden \
	-ffunction-sections \
	-fdata-sections

AM_LDFLAGS = \
	-Wl,--gc-sections \
	-Wl,--as-needed

LIBKMOD_CURRENT=0
LIBKMOD_REVISION=0
LIBKMOD_AGE=0

include_HEADERS = libkmod/libkmod.h
lib_LTLIBRARIES = libkmod/libkmod.la

libkmod_libkmod_la_SOURCES =\
	libkmod/libkmod.h \
	libkmod/libkmod-private.h \
	libkmod/macro.h \
	libkmod/libkmod.c \
	libkmod/libkmod-list.c \
	libkmod/libkmod-hash.c \
	libkmod/libkmod-loaded.c \
	libkmod/libkmod-config.c \
	libkmod/libkmod-util.c \
	libkmod/libkmod-index.c \
	libkmod/libkmod-index.h \
	libkmod/libkmod-module.c

EXTRA_DIST += libkmod/libkmod.sym

libkmod_libkmod_la_LDFLAGS = $(AM_LDFLAGS) \
	-version-info $(LIBKMOD_CURRENT):$(LIBKMOD_REVISION):$(LIBKMOD_AGE) \
	-Wl,--version-script=$(top_srcdir)/libkmod/libkmod.sym

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libkmod/libkmod.pc

TESTS = test/test-init test/test-loaded

check_PROGRAMS = test/test-init test/test-loaded
test_test_init_SOURCES = test/test-init.c
test_test_init_LDADD = libkmod/libkmod.la

test_test_loaded_SOURCES = test/test-loaded.c
test_test_loaded_LDADD = libkmod/libkmod.la

noinst_PROGRAMS = test/test-insmod test/test-rmmod test/test-rmmod2 \
		  test/test-lookup test/test-path-from-name \
		  test/test-get-dependencies test/test-mod-double-ref \
		  $(check_PROGRAMS)

test_test_rmmod_SOURCES = test/test-rmmod.c
test_test_rmmod_LDADD = libkmod/libkmod.la

test_test_rmmod2_SOURCES = test/test-rmmod2.c
test_test_rmmod2_LDADD = libkmod/libkmod.la

test_test_insmod_SOURCES = test/test-insmod.c
test_test_insmod_LDADD = libkmod/libkmod.la

test_test_lookup_SOURCES = test/test-lookup.c
test_test_lookup_LDADD = libkmod/libkmod.la

test_test_path_from_name_SOURCES = test/test-path-from-name.c
test_test_path_from_name_LDADD = libkmod/libkmod.la

test_test_get_dependencies_SOURCES = test/test-get-dependencies.c
test_test_get_dependencies_LDADD = libkmod/libkmod.la

test_test_mod_double_ref_SOURCES = test/test-mod-double-ref.c
test_test_mod_double_ref_LDADD = libkmod/libkmod.la
