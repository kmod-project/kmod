#pragma once

#include <unistd.h>
#include <sys/syscall.h>

/*
 * Macros pulled from linux/module.h, to avoid pulling the header.
 *
 * In practise very few people have it installed and distros do not install the
 * relevant package during their build.
 *
 * Values are UAPI, so they cannot change.
 */
#define MODULE_INIT_IGNORE_MODVERSIONS 1
#define MODULE_INIT_IGNORE_VERMAGIC 2
#define MODULE_INIT_COMPRESSED_FILE 4

#ifndef __NR_finit_module
#warning __NR_finit_module missing - kmod might not work correctly
#define __NR_finit_module -1
#endif

#ifndef HAVE_FINIT_MODULE
#include <errno.h>

static inline int finit_module(int fd, const char *uargs, int flags)
{
	if (__NR_finit_module == -1) {
		errno = ENOSYS;
		return -1;
	}

	return syscall(__NR_finit_module, fd, uargs, flags);
}
#endif

#if !HAVE_DECL_BASENAME
#include <string.h>
static inline const char *basename(const char *s)
{
	const char *p = strrchr(s, '/');
	return p ? p + 1 : s;
}
#endif
