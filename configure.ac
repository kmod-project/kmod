AC_PREREQ(2.60)
AC_INIT([kmod],
	[1],
	[linux-modules@vger.kernel.org],
	[kmod],
	[http://git.profusion.mobi/cgit.cgi/kmod.git/])

AC_CONFIG_SRCDIR([libkmod/libkmod.c])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([check-news foreign 1.11 silent-rules tar-pax dist-bzip2 subdir-objects])
AC_PROG_CC_STDC
AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE
AC_CONFIG_MACRO_DIR([m4])
m4_ifndef([AM_SILENT_RULES], [m4_define([AM_SILENT_RULES],[])])
AM_SILENT_RULES([yes])
LT_INIT([disable-static pic-only])
AC_PREFIX_DEFAULT([/usr])

AC_PROG_CC
AC_PROG_CC_C99
AC_C_TYPEOF
AM_PROG_CC_C_O
AC_PROG_GCC_TRADITIONAL

required_private_libs=""

AC_ARG_ENABLE([tools],
        AS_HELP_STRING([--disable-tools], [disable building tools that provide same functionality as module-init-tools @<:@default=enabled@:>@]),
	[], enable_tools=yes)
AM_CONDITIONAL([BUILD_TOOLS], [test "x$enable_tools" = "xyes"])

AC_ARG_ENABLE([logging],
	AS_HELP_STRING([--disable-logging], [disable system logging @<:@default=enabled@:>@]),
	[], enable_logging=yes)
AS_IF([test "x$enable_logging" = "xyes"], [
	AC_DEFINE(ENABLE_LOGGING, [1], [System logging.])
])

AC_ARG_ENABLE([zlib],
        AS_HELP_STRING([--enable-zlib], [handle gzipped modules (options: static or dynamic) @<:@default=disabled@:>@]),
	[], enable_zlib=no)
if test "x$enable_zlib" = "xyes" -o "x$enable_zlib" = "xstatic"; then
        enable_zlib="static"
        zlib_libs="-Wl,-Bstatic -lz -Wl,-Bdynamic"
        SAVE_LIBS="${LIBS}"
        LIBS="${LIBS} ${zlib_libs}"
        AC_MSG_CHECKING([if static zlib exists])
        AC_LINK_IFELSE([AC_LANG_PROGRAM([[
                #include <zlib.h>
        ]], [[
                gzFile f = gzopen("/tmp", "rb");
        ]])],
        [have_zlib=yes], [have_zlib=no])
        LIBS="${SAVE_LIBS}"
        AC_MSG_RESULT([$have_zlib])
        if test "x$have_zlib" != "xyes"; then
                zlib_libs=""
                AC_MSG_ERROR([static zlib is not present])
        fi
elif test "x$enable_zlib" = "xdynamic"; then
        AC_CHECK_LIB([z], [gzopen],
                [
                        zlib_libs="-lz"
                        required_private_libs="${required_private_libs} ${zlib_libs}"
                ],
                [AC_MSG_ERROR([dynamic zlib is not present])])
else
        AC_MSG_NOTICE([zlib support not requested])
        zlib_libs=""
fi
if test "x$zlib_libs" != "x"; then
        AC_DEFINE(ENABLE_ZLIB, [1], [Enable zlib for modules.])
fi
AC_SUBST(zlib_libs)

AC_ARG_ENABLE([debug],
	AS_HELP_STRING([--enable-debug], [enable debug messages @<:@default=disabled@:>@]),
	[], [enable_debug=no])
AS_IF([test "x$enable_debug" = "xyes"], [
	AC_DEFINE(ENABLE_DEBUG, [1], [Debug messages.])
])

CC_CHECK_CFLAGS_APPEND([ \
			-pipe \
			-Wall \
			-W \
			-Wextra \
			-Wno-inline \
			-Wvla \
			-Wundef \
			-Wformat=2 \
			-Wlogical-op \
			-Wsign-compare \
			-Wformat-security \
			-Wmissing-include-dirs \
			-Wformat-nonliteral \
			-Wold-style-definition \
			-Wpointer-arith \
			-Winit-self \
			-Wdeclaration-after-statement \
			-Wfloat-equal \
			-Wmissing-prototypes \
			-Wstrict-prototypes \
			-Wredundant-decls \
			-Wmissing-declarations \
			-Wmissing-noreturn \
			-Wshadow \
			-Wendif-labels \
			-Wcast-align \
			-Wstrict-aliasing=2 \
			-Wwrite-strings \
			-Wno-long-long \
			-Wno-overlength-strings \
			-Wno-unused-parameter \
			-Wno-missing-field-initializers \
			-Wno-unused-result \
			-Wnested-externs \
			-Wchar-subscripts \
			-Wtype-limits \
			-Wuninitialized \
			-Wp,-D_FORTIFY_SOURCE=2 \
			-ffast-math \
			-fno-common \
			-fdiagnostics-show-option \
			-fno-strict-aliasing \
			-fvisibility=hidden \
			-ffunction-sections \
			-fdata-sections \
			-Wl,--as-needed \
			-Wl,--gc-sections])


AC_SUBST(required_private_libs)

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES([
	Makefile
	libkmod/libkmod.pc
])

AC_OUTPUT
AC_MSG_RESULT([
	$PACKAGE $VERSION
	========

	prefix:			${prefix}
	sysconfdir:		${sysconfdir}
	libdir:			${libdir}
	includedir:		${includedir}
	bindir:			${bindir}

	compiler:		${CC}
	cflags:			${CFLAGS}
	ldflags:		${LDFLAGS}

	tools:			${enable_tools}
	logging:		${enable_logging}
        zlib:                   ${enable_zlib}
	debug:			${enable_debug}
])
