// SPDX-License-Identifier: LGPL-2.1-or-later

#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <signal.h>
#include <stddef.h>

/*
 * Workaround for objtool (tools/objtool/signal.c in Linux >= 6.18) using a
 * compile-time SIGSTKSZ (8192 bytes on musl, or on glibc without _GNU_SOURCE).
 * On CPUs with larger XSAVE states (e.g. Intel AMX where AT_MINSIGSTKSZ > 8192),
 * the kernel rejects the 8 KiB stack in do_sigaltstack() with ENOMEM.
 */
int sigaltstack(const stack_t *restrict ss, stack_t *restrict old_ss)
{
	static _Alignas(max_align_t) unsigned char altstack[1U << 17];
	static int (*next_sigaltstack)(const stack_t *restrict, stack_t *restrict);
	stack_t new_ss;

	if (next_sigaltstack == NULL) {
		next_sigaltstack = dlsym(RTLD_NEXT, "sigaltstack");
		if (next_sigaltstack == NULL) {
			errno = ENOSYS;
			return -1;
		}
	}

	if (ss == NULL || (ss->ss_flags & SS_DISABLE) || ss->ss_size >= sizeof(altstack))
		return next_sigaltstack(ss, old_ss);

	new_ss = *ss;
	new_ss.ss_sp = altstack;
	new_ss.ss_size = sizeof(altstack);

	return next_sigaltstack(&new_ss, old_ss);
}
