AC_PREREQ(2.64)
AC_INIT([kmod],
	[32],
	[linux-modules@vger.kernel.org],
	[kmod],
	[http://git.kernel.org/?p=utils/kernel/kmod/kmod.git])

AC_CONFIG_SRCDIR([libkmod/libkmod.c])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_AUX_DIR([build-aux])

AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE
AC_PREFIX_DEFAULT([/usr])
AM_MAINTAINER_MODE([enable])
AM_INIT_AUTOMAKE([check-news foreign 1.11 silent-rules tar-pax no-dist-gzip dist-xz subdir-objects color-tests parallel-tests])
AM_SILENT_RULES([yes])
LT_INIT([disable-static pic-only])

AS_IF([test "x$enable_static" = "xyes"], [AC_MSG_ERROR([--enable-static is not supported by kmod])])
AS_IF([test "x$enable_largefile" = "xno"], [AC_MSG_ERROR([--disable-largefile is not supported by kmod])])

module_compressions=""
module_signatures="legacy"

#####################################################################
# Program checks and configurations
#####################################################################

AC_PROG_SED
AC_PROG_MKDIR_P
AC_PROG_LN_S
PKG_PROG_PKG_CONFIG
AC_PATH_PROG([XSLTPROC], [xsltproc])

AC_PROG_CC_C99

#####################################################################
# Function and structure checks
#####################################################################

AC_CHECK_FUNCS_ONCE(__xstat)
AC_CHECK_FUNCS_ONCE([__secure_getenv secure_getenv])
AC_CHECK_FUNCS_ONCE([finit_module])

CC_CHECK_FUNC_BUILTIN([__builtin_clz])
CC_CHECK_FUNC_BUILTIN([__builtin_types_compatible_p])
CC_CHECK_FUNC_BUILTIN([__builtin_uaddl_overflow], [ ], [ ])
CC_CHECK_FUNC_BUILTIN([__builtin_uaddll_overflow], [ ], [ ])

# dietlibc doesn't have st.st_mtim struct member
AC_CHECK_MEMBERS([struct stat.st_mtim], [], [], [#include <sys/stat.h>])

# musl 1.0 and bionic 4.4 don't have strndupa
AC_CHECK_DECLS_ONCE([strndupa])

# RHEL 5 and older do not have be32toh
AC_CHECK_DECLS_ONCE([be32toh])

# Check kernel headers
AC_CHECK_HEADERS_ONCE([linux/module.h])

AC_MSG_CHECKING([whether _Static_assert() is supported])
AC_COMPILE_IFELSE(
	[AC_LANG_SOURCE([[_Static_assert(1, "Test");]])],
        [AC_DEFINE([HAVE_STATIC_ASSERT], [1], [Define if _Static_assert() is available])
	 AC_MSG_RESULT([yes])],
	[AC_MSG_RESULT([no])])

AC_MSG_CHECKING([whether _Noreturn is supported])
AC_COMPILE_IFELSE(
	[AC_LANG_SOURCE([[#include <stdlib.h>
	_Noreturn int foo(void) { exit(0); }]])],
        [AC_DEFINE([HAVE_NORETURN], [1], [Define if _Noreturn is available])
	 AC_MSG_RESULT([yes])],
	[AC_MSG_RESULT([no])])


#####################################################################
# --with-
#####################################################################

AC_ARG_WITH([distconfdir], AS_HELP_STRING([--with-distconfdir=DIR], [directory to search for distribution configuration files]),
        [], [with_distconfdir='${prefix}/lib'])
AC_SUBST([distconfdir], [$with_distconfdir])

AC_ARG_WITH([rootlibdir],
        AS_HELP_STRING([--with-rootlibdir=DIR], [rootfs directory to install shared libraries]),
        [], [with_rootlibdir=$libdir])
AC_SUBST([rootlibdir], [$with_rootlibdir])

# Ideally this would be $prefix/lib/modules but default to /lib/modules for compatibility with earlier versions
AC_ARG_WITH([module_directory],
        AS_HELP_STRING([--with-module-directory=DIR], [directory in which to look for kernel modules @<:@default=/lib/modules@:>@]),
        [], [with_module_directory=/lib/modules])
AC_SUBST([module_directory], [$with_module_directory])

# Check all directory arguments for consistency.
for ac_var in	distconfdir rootlibdir module_directory
do
  eval ac_val=\$$ac_var
  # Remove trailing slashes.
  case $ac_val in
    */ )
      ac_val=`expr "X$ac_val" : 'X\(.*@<:@^/@:>@\)' \| "X$ac_val" : 'X\(.*\)'`
      eval $ac_var=\$ac_val;;
  esac
  # Be sure to have absolute directory names.
  case $ac_val in
    @<:@\\/$@:>@* | ?:@<:@\\/@:>@* )  continue;;
  esac
  as_fn_error $? "expected an absolute directory name for --$ac_var: $ac_val"
done

AC_ARG_WITH([zstd],
	AS_HELP_STRING([--with-zstd], [handle Zstandard-compressed modules @<:@default=disabled@:>@]),
	[], [with_zstd=no])
AS_IF([test "x$with_zstd" != "xno"], [
	PKG_CHECK_MODULES([libzstd], [libzstd >= 1.4.4], [LIBS="$LIBS $libzstd_LIBS"])
	AC_DEFINE([ENABLE_ZSTD], [1], [Enable Zstandard for modules.])
	module_compressions="zstd $module_compressions"
], [
	AC_MSG_NOTICE([Zstandard support not requested])
])
CC_FEATURE_APPEND([with_features], [with_zstd], [ZSTD])

AC_ARG_WITH([xz],
	AS_HELP_STRING([--with-xz], [handle Xz-compressed modules @<:@default=disabled@:>@]),
	[], [with_xz=no])
AS_IF([test "x$with_xz" != "xno"], [
	PKG_CHECK_MODULES([liblzma], [liblzma >= 4.99], [LIBS="$LIBS $liblzma_LIBS"])
	AC_DEFINE([ENABLE_XZ], [1], [Enable Xz for modules.])
	module_compressions="xz $module_compressions"
], [
	AC_MSG_NOTICE([Xz support not requested])
])
CC_FEATURE_APPEND([with_features], [with_xz], [XZ])

AC_ARG_WITH([zlib],
	AS_HELP_STRING([--with-zlib], [handle gzipped modules @<:@default=disabled@:>@]),
	[], [with_zlib=no])
AS_IF([test "x$with_zlib" != "xno"], [
	PKG_CHECK_MODULES([zlib], [zlib], [LIBS="$LIBS $zlib_LIBS"])
	AC_DEFINE([ENABLE_ZLIB], [1], [Enable zlib for modules.])
	module_compressions="gzip $module_compressions"
], [
	AC_MSG_NOTICE([zlib support not requested])
])
CC_FEATURE_APPEND([with_features], [with_zlib], [ZLIB])

AC_ARG_WITH([openssl],
	AS_HELP_STRING([--with-openssl], [handle PKCS7 signatures @<:@default=disabled@:>@]),
	[], [with_openssl=no])
AS_IF([test "x$with_openssl" != "xno"], [
	PKG_CHECK_MODULES([libcrypto], [libcrypto >= 1.1.0], [LIBS="$LIBS $libcrypto_LIBS"])
	AC_DEFINE([ENABLE_OPENSSL], [1], [Enable openssl for modinfo.])
	module_signatures="PKCS7 $module_signatures"
], [
	AC_MSG_NOTICE([openssl support not requested])
])
CC_FEATURE_APPEND([with_features], [with_openssl], [LIBCRYPTO])

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

AC_ARG_ENABLE([test-modules],
        AS_HELP_STRING([--disable-test-modules], [disable building test modules during make check: cached modules will be used @<:@default=enabled@:>@]),
	[], enable_test_modules=yes)
AM_CONDITIONAL([BUILD_MODULES], [test "x$enable_test_modules" = "xyes"])

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

AC_ARG_ENABLE([coverage],
	AS_HELP_STRING([--enable-coverage], [enable test coverage @<:@default=disabled@:>@]),
	[], [enable_coverage=no])
AS_IF([test "x$enable_coverage" = "xyes"], [
	AC_CHECK_PROG(have_coverage, [lcov], [yes], [no])
	AS_IF([test "x$have_coverage" = xno],[
		AC_MSG_ERROR([*** lcov support requested but the program was not found])
	], [
		lcov_version_major="`lcov --version | cut -d ' ' -f 4 | cut -d '.' -f 1`"
		lcov_version_minor="`lcov --version | cut -d ' ' -f 4 | cut -d '.' -f 2`"
		AS_IF([test "$lcov_version_major" -lt 1 -o "$lcov_version_minor" -lt 10], [
			AC_MSG_ERROR([*** lcov version is too old. 1.10 required])
		], [
			have_coverage=yes
                        CC_CHECK_FLAGS_APPEND([with_coverage_cflags], [CFLAGS], [\
                        -fprofile-arcs \
                        -ftest-coverage])
		])
        ])
])
AM_CONDITIONAL([ENABLE_COVERAGE], [test "x$enable_coverage" = "xyes"])

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
		       -Wstrict-aliasing=3 \
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
AC_SUBST([OUR_CFLAGS], "$with_cflags $with_coverage_cflags")


CC_CHECK_FLAGS_APPEND([with_ldflags], [LDFLAGS], [ \
		       -Wl,--as-needed \
		       -Wl,--no-undefined \
		       -Wl,--gc-sections])
AC_SUBST([OUR_LDFLAGS], $with_ldflags)

AC_DEFINE_UNQUOTED(KMOD_FEATURES, ["$with_features"], [Features in this build])

#####################################################################
# Generate files from *.in
#####################################################################

AC_SUBST([module_compressions], $module_compressions)
AC_SUBST([module_signatures], $module_signatures)

AC_CONFIG_FILES([
	Makefile
	man/Makefile
	libkmod/docs/Makefile
	libkmod/docs/version.xml
	libkmod/libkmod.pc
	tools/kmod.pc
])


#####################################################################

AC_OUTPUT
AC_MSG_RESULT([
	$PACKAGE $VERSION
	=======

	module_directory:	${module_directory}
	prefix:			${prefix}
	sysconfdir:		${sysconfdir}
	distconfdir:		${distconfdir}
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
	compression:		zstd=${with_zstd}  xz=${with_xz}  zlib=${with_zlib}
	debug:			${enable_debug}
	coverage:		${enable_coverage}
	doc:			${enable_gtk_doc}
	man:			${enable_manpages}
	test-modules:           ${enable_test_modules}

	features:               ${with_features}
])
