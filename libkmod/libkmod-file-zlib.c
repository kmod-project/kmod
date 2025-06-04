// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright © 2024 Intel Corporation
 */

#define DLSYM_LOCALLY_ENABLED ENABLE_ZLIB_DLOPEN

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <zlib.h>

#include <shared/elf-note.h>
#include <shared/util.h>

#include "libkmod.h"
#include "libkmod-internal.h"
#include "libkmod-internal-file.h"

#define READ_STEP (4 * 1024 * 1024)

#define DL_SYMBOL_TABLE(M) \
	M(gzclose)         \
	M(gzdopen)         \
	M(gzerror)         \
	M(gzread)

DL_SYMBOL_TABLE(DECLARE_SYM)

static int dlopen_zlib(void)
{
#if !DLSYM_LOCALLY_ENABLED
	return 0;
#else
	static void *dl;

	ELF_NOTE_DLOPEN("zlib", "Support for uncompressing zlib-compressed modules",
			ELF_NOTE_DLOPEN_PRIORITY_RECOMMENDED, "libz.so.1");

	return dlsym_many(&dl, "libz.so.1", DL_SYMBOL_TABLE(DLSYM_ARG) NULL);
#endif
}

int kmod_file_load_zlib(struct kmod_file *file)
{
	_cleanup_free_ unsigned char *p = NULL;
	int ret = 0;
	off_t did = 0, total = 0;
	gzFile gzf;
	int gzfd;

	ret = dlopen_zlib();
	if (ret < 0) {
		ERR(file->ctx, "zlib: can't load and resolve symbols (%s)",
		    strerror(-ret));
		return -EINVAL;
	}

	gzfd = fcntl(file->fd, F_DUPFD_CLOEXEC, 3);
	if (gzfd < 0)
		return -errno;

	gzf = sym_gzdopen(gzfd, "rb"); /* takes ownership of the fd */
	if (gzf == NULL) {
		close(gzfd);
		return -errno;
	}

	for (;;) {
		int r;

		if (did == total) {
			void *tmp = realloc(p, total + READ_STEP);
			if (tmp == NULL) {
				ret = -ENOMEM;
				goto error;
			}
			total += READ_STEP;
			p = tmp;
		}

		r = sym_gzread(gzf, p + did, total - did);
		if (r == 0)
			break;
		else if (r < 0) {
			int gzerr;
			const char *gz_errmsg = sym_gzerror(gzf, &gzerr);

			ERR(file->ctx, "gzip: %s\n", gz_errmsg);

			/* gzip might not set errno here */
			ret = gzerr == Z_ERRNO ? -errno : -EINVAL;
			goto error;
		}
		did += r;
	}

	file->memory = TAKE_PTR(p);
	file->size = did;
	sym_gzclose(gzf);

	return 0;

error:
	sym_gzclose(gzf);
	return ret;
}
