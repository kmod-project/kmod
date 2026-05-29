#pragma once

#include <unistd.h>
#include <sys/syscall.h>

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
