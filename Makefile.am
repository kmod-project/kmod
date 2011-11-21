EXTRA_DIST = autogen.sh
ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}
AM_MAKEFLAGS = --no-print-directory

AM_CPPFLAGS = \
	-include $(top_builddir)/config.h \
	-I$(top_srcdir)/libabc \
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

LIBABC_CURRENT=2
LIBABC_REVISION=0
LIBABC_AGE=2

include_HEADERS = libabc/libabc.h
lib_LTLIBRARIES = libabc/libabc.la

libabc_libabc_la_SOURCES =\
	libabc/libabc.h \
	libabc/libabc-private.h \
	libabc/libabc.c

EXTRA_DIST += libabc/libabc.sym

libabc_libabc_la_LDFLAGS = $(AM_LDFLAGS) \
	-version-info $(LIBABC_CURRENT):$(LIBABC_REVISION):$(LIBABC_AGE) \
	-Wl,--version-script=$(top_srcdir)/libabc/libabc.sym

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libabc/libabc.pc

TESTS = test-libabc

check_PROGRAMS = test-libabc
test_libabc_SOURCES = test-libabc.c
test_libabc_LDADD = libabc/libabc.la
