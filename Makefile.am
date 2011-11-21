EXTRA_DIST = autogen.sh
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

CLEANFILES =

LIBKMOD_CURRENT=2
LIBKMOD_REVISION=0
LIBKMOD_AGE=2

include_HEADERS = libkmod/libkmod.h
lib_LTLIBRARIES = libkmod/libkmod.la

libkmod_libkmod_la_SOURCES =\
	libkmod/libkmod.h \
	libkmod/libkmod-private.h \
	libkmod/libkmod.c

EXTRA_DIST += libkmod/libkmod.sym

libkmod_libkmod_la_LDFLAGS = $(AM_LDFLAGS) \
	-version-info $(LIBKMOD_CURRENT):$(LIBKMOD_REVISION):$(LIBKMOD_AGE) \
	-Wl,--version-script=$(top_srcdir)/libkmod/libkmod.sym

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libkmod/libkmod.pc

# TESTS = test-libkmod
#
# check_PROGRAMS = test-libkmod
# test_libkmod_SOURCES = test-libkmod.c
# test_libkmod_LDADD = libkmod/libkmod.la
