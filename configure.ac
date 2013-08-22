AC_PREREQ(2.60)
AC_INIT([kmod],
	[15],
	[linux-modules@vger.kernel.org],
	[kmod],
	[http://git.kernel.org/?p=utils/kernel/kmod/kmod.git])

AC_CONFIG_SRCDIR([libkmod/libkmod.c])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([check-news foreign 1.11 silent-rules
	tar-pax no-dist-gzip dist-xz subdir-objects color-tests parallel-tests])
AC_PROG_CC_STDC
AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE
AC_CONFIG_MACRO_DIR([m4])
m4_ifndef([AM_SILENT_RULES], [m4_define([AM_SILENT_RULES],[])])
AM_SILENT_RULES([yes])
LT_INIT([disable-static pic-only])
AC_PREFIX_DEFAULT([/usr])

AS_IF([test "x$enable_static" = "xyes"],
      [AC_MSG_ERROR([--enable-static is not supported by kmod])])


#####################################################################
# Program checks and configurations
#####################################################################

AC_PROG_CC
AC_PROG_CC_C99
AC_C_TYPEOF
AM_PROG_CC_C_O
AC_PROG_GCC_TRADITIONAL
AC_C_BIGENDIAN

AC_PROG_SED
AC_PROG_MKDIR_P
PKG_PROG_PKG_CONFIG


#####################################################################
# Function and structure checks
#####################################################################

AC_CHECK_FUNCS_ONCE(__xstat)
AC_CHECK_FUNCS_ONCE([__secure_getenv secure_getenv])
AC_CHECK_FUNCS_ONCE([finit_module])

# dietlibc doesn't have st.st_mtim struct member
AC_CHECK_MEMBERS([struct stat.st_mtim], [], [], [#include <sys/stat.h>])

# Check kernel headers
AC_CHECK_HEADERS_ONCE([linux/module.h])


#####################################################################
# --with-
#####################################################################

AC_ARG_WITH([rootlibdir],
        AS_HELP_STRING([--with-rootlibdir=DIR], [rootfs directory to install shared libraries]),
        [], [with_rootlibdir=$libdir])
AC_SUBST([rootlibdir], [$with_rootlibdir])

AC_ARG_WITH([xz],
	AS_HELP_STRING([--with-xz], [handle Xz-compressed modules @<:@default=disabled@:>@]),
	[], [with_xz=no])
AS_IF([test "x$with_xz" != "xno"], [
	PKG_CHECK_MODULES([liblzma], [liblzma >= 4.99])
	AC_DEFINE([ENABLE_XZ], [1], [Enable Xz for modules.])
], [
	AC_MSG_NOTICE([Xz support not requested])
])

AC_ARG_WITH([zlib],
	AS_HELP_STRING([--with-zlib], [handle gzipped modules @<:@default=disabled@:>@]),
	[], [with_zlib=no])
AS_IF([test "x$with_zlib" != "xno"], [
	PKG_CHECK_MODULES([zlib], [zlib])
	AC_DEFINE([ENABLE_ZLIB], [1], [Enable zlib for modules.])
], [
	AC_MSG_NOTICE([zlib support not requested])
])

AC_ARG_WITH([bashcompletiondir],
	AS_HELP_STRING([--with-bashcompletiondir=DIR], [Bash completions directory]),
	[],
	[AS_IF([$($PKG_CONFIG --exists bash-completion)], [
		with_bashcompletiondir=$($PKG_CONFIG --variable=completionsdir bash-completion)
	] , [
		with_bashcompletiondir=${datadir}/bash-completion/completions
	])])
AC_SUBST([bashcompletiondir], [$with_bashcompletiondir])

#####################################################################
# --enable-
#####################################################################

AC_ARG_ENABLE([tools],
        AS_HELP_STRING([--disable-tools], [disable building tools that provide same functionality as module-init-tools @<:@default=enabled@:>@]),
	[], enable_tools=yes)
AM_CONDITIONAL([BUILD_TOOLS], [test "x$enable_tools" = "xyes"])

AC_ARG_ENABLE([manpages],
        AS_HELP_STRING([--disable-manpages], [disable manpages @<:@default=enabled@:>@]),
	[], enable_manpages=yes)
AM_CONDITIONAL([BUILD_MANPAGES], [test "x$enable_manpages" = "xyes"])
AC_PATH_PROG([XSLTPROC], [xsltproc], [no])
AS_IF([test "x$XSLTPROC" = "xno" && test "x$enable_manpages" = "xyes"], [
	AC_MSG_ERROR([xsltproc command not found, try ./configure --disable-manpages])
])

AC_ARG_ENABLE([logging],
	AS_HELP_STRING([--disable-logging], [disable system logging @<:@default=enabled@:>@]),
	[], enable_logging=yes)
AS_IF([test "x$enable_logging" = "xyes"], [
	AC_DEFINE(ENABLE_LOGGING, [1], [System logging.])
])

AC_ARG_ENABLE([debug],
	AS_HELP_STRING([--enable-debug], [enable debug messages @<:@default=disabled@:>@]),
	[], [enable_debug=no])
AS_IF([test "x$enable_debug" = "xyes"], [
	AC_DEFINE(ENABLE_DEBUG, [1], [Debug messages.])
])

m4_ifdef([GTK_DOC_CHECK], [
GTK_DOC_CHECK([1.14],[--flavour no-tmpl-flat])
], [
AM_CONDITIONAL([ENABLE_GTK_DOC], false)])


#####################################################################
# Default CFLAGS and LDFLAGS
#####################################################################

CC_CHECK_FLAGS_APPEND(with_cflags, [CFLAGS], [\
		       -pipe \
		       -DANOTHER_BRICK_IN_THE \
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
		       -fno-common \
		       -fdiagnostics-show-option \
		       -fvisibility=hidden \
		       -ffunction-sections \
		       -fdata-sections])
AC_SUBST([WARNINGFLAGS], $with_cflags)


CC_CHECK_FLAGS_APPEND([with_ldflags], [LDFLAGS], [ \
		       -Wl,--as-needed \
		       -Wl,--gc-sections])
AC_SUBST([GCLDFLAGS], $with_ldflags)

#####################################################################
# Generate files from *.in
#####################################################################

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES([
	Makefile
	man/Makefile
	libkmod/docs/Makefile
	libkmod/docs/version.xml
])


#####################################################################

AC_OUTPUT
AC_MSG_RESULT([
	$PACKAGE $VERSION
	=======

	prefix:			${prefix}
	sysconfdir:		${sysconfdir}
	libdir:			${libdir}
	rootlibdir:		${rootlibdir}
	includedir:		${includedir}
	bindir:			${bindir}
	Bash completions dir:   ${with_bashcompletiondir}

	compiler:		${CC}
	cflags:			${with_cflags} ${CFLAGS}
	ldflags:		${with_ldflags} ${LDFLAGS}

	tools:			${enable_tools}
	logging:		${enable_logging}
	compression:		xz=${with_xz}  zlib=${with_zlib}
	debug:			${enable_debug}
	doc:			${enable_gtk_doc}
	man:			${enable_manpages}
])
