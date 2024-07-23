// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2012-2013  ProFUSION embedded systems
 */

/* We unset _FILE_OFFSET_BITS here so we can override both stat and stat64 on
 * 32-bit architectures and forward each to the right libc function */
#undef _FILE_OFFSET_BITS
#undef _TIME_BITS

#include <assert.h>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <shared/util.h>

#include "testsuite.h"

static void *nextlib;
static const char *rootpath;
static size_t rootpathlen;

static inline bool need_trap(const char *path)
{
	return path != NULL && path[0] == '/'
		&& !strstartswith(path, ABS_TOP_BUILDDIR);
}

static const char *trap_path(const char *path, char buf[PATH_MAX * 2])
{
	size_t len;

	if (!need_trap(path))
		return path;

	len = strlen(path);

	if (len + rootpathlen > PATH_MAX * 2) {
		errno = ENAMETOOLONG;
		return NULL;
	}

	memcpy(buf, rootpath, rootpathlen);
	strcpy(buf + rootpathlen, path);
	return buf;
}

static bool get_rootpath(const char *f)
{
	if (rootpath != NULL)
		return true;

	rootpath = getenv(S_TC_ROOTFS);
	if (rootpath == NULL) {
		ERR("TRAP %s(): missing export %s?\n", f, S_TC_ROOTFS);
		errno = ENOENT;
		return false;
	}

	rootpathlen = strlen(rootpath);

	return true;
}

static void *get_libc_func(const char *f)
{
	void *fp;

	if (nextlib == NULL) {
#ifdef RTLD_NEXT
		nextlib = RTLD_NEXT;
#else
		nextlib = dlopen("libc.so.6", RTLD_LAZY);
#endif
	}

	fp = dlsym(nextlib, f);
	assert(fp);

	return fp;
}

/* wrapper template for a function with one "const char* path" argument */
#define WRAP_1ARG(rettype, failret, name) \
TS_EXPORT rettype name(const char *path) \
{ \
	const char *p;				\
	char buf[PATH_MAX * 2];                 \
	static rettype (*_fn)(const char*);	\
						\
	if (!get_rootpath(__func__))		\
		return failret;			\
	_fn = get_libc_func(#name);		\
	p = trap_path(path, buf);		\
	if (p == NULL)				\
		return failret;			\
	return (*_fn)(p);			\
}

/* wrapper template for a function with "const char* path" and another argument */
#define WRAP_2ARGS(rettype, failret, name, arg2t)	\
TS_EXPORT rettype name(const char *path, arg2t arg2)	\
{ \
	const char *p;					\
	char buf[PATH_MAX * 2];				\
	static rettype (*_fn)(const char*, arg2t arg2);	\
							\
	if (!get_rootpath(__func__))			\
		return failret;				\
	_fn = get_libc_func(#name);			\
	p = trap_path(path, buf);			\
	if (p == NULL)					\
		return failret;				\
	return (*_fn)(p, arg2);				\
}

/* wrapper template for open family */
#define WRAP_OPEN(suffix)					\
TS_EXPORT int open ## suffix (const char *path, int flags, ...)	\
{ \
	const char *p;						\
	char buf[PATH_MAX * 2];					\
	static int (*_fn)(const char *path, int flags, ...);	\
								\
	if (!get_rootpath(__func__))				\
		return -1;					\
	_fn = get_libc_func("open" #suffix);			\
	p = trap_path(path, buf);				\
	if (p == NULL)						\
		return -1;					\
								\
	if (flags & O_CREAT) {					\
		mode_t mode;					\
		va_list ap;					\
								\
		va_start(ap, flags);				\
		mode = va_arg(ap, mode_t);			\
		va_end(ap);					\
		return _fn(p, flags, mode);			\
	}							\
								\
	return _fn(p, flags);					\
}

/*
 * wrapper template for __xstat family
 * This family got deprecated/dropped in glibc 2.32.9000, but we still need
 * to keep it for a while for programs that were built against previous versions
 */
#define WRAP_VERSTAT(prefix, suffix)			    \
TS_EXPORT int prefix ## stat ## suffix (int ver,	    \
			      const char *path,		    \
	                      struct stat ## suffix *st);   \
TS_EXPORT int prefix ## stat ## suffix (int ver,	    \
			      const char *path,		    \
	                      struct stat ## suffix *st)    \
{ \
	const char *p;					    \
	char buf[PATH_MAX * 2];				    \
	static int (*_fn)(int ver, const char *path,	    \
		          struct stat ## suffix *);	    \
	_fn = get_libc_func(#prefix "stat" #suffix);	    \
							    \
	if (!get_rootpath(__func__))			    \
		return -1;				    \
	p = trap_path(path, buf);			    \
	if (p == NULL)					    \
		return -1;				    \
							    \
	return _fn(ver, p, st);				    \
}

WRAP_1ARG(DIR*, NULL, opendir);
WRAP_1ARG(int, -1, chdir);

WRAP_2ARGS(FILE*, NULL, fopen, const char*);
WRAP_2ARGS(FILE*, NULL, fopen64, const char*);
WRAP_2ARGS(int, -1, mkdir, mode_t);
WRAP_2ARGS(int, -1, access, int);
WRAP_2ARGS(int, -1, stat, struct stat*);
WRAP_2ARGS(int, -1, lstat, struct stat*);
WRAP_2ARGS(int, -1, stat64, struct stat64*);
WRAP_2ARGS(int, -1, lstat64, struct stat64*);

struct __stat64_t64;
extern int __stat64_time64 (const char *file, struct __stat64_t64 *buf);
extern int __lstat64_time64 (const char *file, struct __stat64_t64 *buf);
WRAP_2ARGS(int, -1, __stat64_time64, struct __stat64_t64*);
WRAP_2ARGS(int, -1, __lstat64_time64, struct __stat64_t64*);

WRAP_OPEN(64);
WRAP_OPEN();

#ifdef HAVE___XSTAT
WRAP_VERSTAT(__x,);
WRAP_VERSTAT(__lx,);
WRAP_VERSTAT(__x,64);
WRAP_VERSTAT(__lx,64);
#endif
