#pragma once

#include <unistd.h>
#include <sys/syscall.h>

#ifdef HAVE_LINUX_MODULE_H
#include <linux/module.h>
#endif

#ifndef MODULE_INIT_IGNORE_MODVERSIONS
# define MODULE_INIT_IGNORE_MODVERSIONS 1
#endif

#ifndef MODULE_INIT_IGNORE_VERMAGIC
# define MODULE_INIT_IGNORE_VERMAGIC 2
#endif

#ifndef __NR_finit_module
# define __NR_finit_module -1
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
