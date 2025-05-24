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

static inline bool need_trap(const char *path)
{
	/*
	 * Always consider the ABS_TOP_BUILDDIR as the base root of anything we do.
	 *
	 * Changing this to rootpath is tempting but incorrect, since it won't consider
	 * any third-party files managed through this LD_PRELOAD library, eg coverage.
	 *
	 * In other words: if using rootpath, the coverage gcna files will end up created
	 * in the wrong location eg. $rootpath/$ABS_TOP_BUILDDIR.
	 */
	return path != NULL && path[0] == '/' && !strstartswith(path, ABS_TOP_BUILDDIR);
}

static const char *trap_path(const char *path, char buf[PATH_MAX * 2])
{
	static const char *rootpath;
	static size_t rootpathlen;
	size_t len;

	if (!need_trap(path))
		return path;

	len = strlen(path);

	if (rootpath == NULL) {
		rootpath = getenv(S_TC_ROOTFS);
		if (rootpath == NULL) {
			ERR("TRAP: missing export %s?\n", S_TC_ROOTFS);
			errno = ENOENT;
			return NULL;
		}

		rootpathlen = strlen(rootpath);
	}

	if (len + rootpathlen > PATH_MAX * 2) {
		errno = ENAMETOOLONG;
		return NULL;
	}

	memcpy(buf, rootpath, rootpathlen);
	strcpy(buf + rootpathlen, path);
	return buf;
}

static void *get_libc_func(const char *f)
{
	void *fp;

	fp = dlsym(RTLD_NEXT, f);
	if (fp == NULL) {
		fprintf(stderr, "FIXME FIXME FIXME: could not load %s symbol: %s\n", f,
			dlerror());
		abort();
	}

	return fp;
}

/* wrapper template for a function with one "const char* path" argument */
#define WRAP_1ARG(rettype, failret, name)            \
	TS_EXPORT rettype name(const char *path)     \
	{                                            \
		const char *p;                       \
		char buf[PATH_MAX * 2];              \
		static rettype (*_fn)(const char *); \
                                                     \
		p = trap_path(path, buf);            \
		if (p == NULL)                       \
			return failret;              \
                                                     \
		if (_fn == NULL)                     \
			_fn = get_libc_func(#name);  \
		return (*_fn)(p);                    \
	}

/* wrapper template for a function with "const char* path" and another argument */
#define WRAP_2ARGS(rettype, failret, name, arg2t)                \
	TS_EXPORT rettype name(const char *path, arg2t arg2)     \
	{                                                        \
		const char *p;                                   \
		char buf[PATH_MAX * 2];                          \
		static rettype (*_fn)(const char *, arg2t arg2); \
                                                                 \
		p = trap_path(path, buf);                        \
		if (p == NULL)                                   \
			return failret;                          \
                                                                 \
		if (_fn == NULL)                                 \
			_fn = get_libc_func(#name);              \
		return (*_fn)(p, arg2);                          \
	}

/* wrapper template for open family */
#define WRAP_OPEN(suffix)                                            \
	TS_EXPORT int open##suffix(const char *path, int flags, ...) \
	{                                                            \
		const char *p;                                       \
		char buf[PATH_MAX * 2];                              \
		static int (*_fn)(const char *path, int flags, ...); \
                                                                     \
		p = trap_path(path, buf);                            \
		if (p == NULL)                                       \
			return -1;                                   \
                                                                     \
		if (_fn == NULL)                                     \
			_fn = get_libc_func("open" #suffix);         \
		if (flags & O_CREAT) {                               \
			mode_t mode;                                 \
			va_list ap;                                  \
                                                                     \
			va_start(ap, flags);                         \
			mode = va_arg(ap, mode_t);                   \
			va_end(ap);                                  \
			return _fn(p, flags, mode);                  \
		}                                                    \
                                                                     \
		return _fn(p, flags);                                \
	}

#define WRAP_VERSTAT(prefix, suffix)                                                 \
	TS_EXPORT int prefix##stat##suffix(int ver, const char *path,                \
					   struct stat##suffix *st)                  \
	{                                                                            \
		const char *p;                                                       \
		char buf[PATH_MAX * 2];                                              \
		static int (*_fn)(int ver, const char *path, struct stat##suffix *); \
                                                                                     \
		p = trap_path(path, buf);                                            \
		if (p == NULL)                                                       \
			return -1;                                                   \
                                                                                     \
		if (_fn == NULL)                                                     \
			_fn = get_libc_func(#prefix "stat" #suffix);                 \
		return _fn(ver, p, st);                                              \
	}

WRAP_1ARG(DIR *, NULL, opendir);
WRAP_1ARG(int, -1, chdir);
WRAP_1ARG(int, -1, remove);
WRAP_1ARG(int, -1, rmdir);

WRAP_2ARGS(FILE *, NULL, fopen, const char *);
WRAP_2ARGS(int, -1, mkdir, mode_t);
WRAP_2ARGS(int, -1, stat, struct stat *);

WRAP_OPEN();

#if HAVE_FOPEN64
WRAP_2ARGS(FILE *, NULL, fopen64, const char *);
#endif
#if HAVE_STAT64
WRAP_2ARGS(int, -1, stat64, struct stat64 *);
#endif

#if HAVE___STAT64_TIME64
extern int __stat64_time64(const char *file, void *buf);
WRAP_2ARGS(int, -1, __stat64_time64, void *);
#endif

#if HAVE_OPEN64
WRAP_OPEN(64);
#endif

#if HAVE_DECL___XSTAT
WRAP_VERSTAT(__x, );
WRAP_VERSTAT(__x, 64);
#endif
