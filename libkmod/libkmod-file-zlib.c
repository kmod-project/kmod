// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright © 2024 Intel Corporation
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <zlib.h>

#include <shared/util.h>

#include "libkmod.h"
#include "libkmod-internal.h"
#include "libkmod-internal-file.h"

#define READ_STEP (4 * 1024 * 1024)

int kmod_file_load_zlib(struct kmod_file *file)
{
	int err = 0;
	off_t did = 0, total = 0;
	_cleanup_free_ unsigned char *p = NULL;
	gzFile gzf;
	int gzfd;

	errno = 0;
	gzfd = fcntl(file->fd, F_DUPFD_CLOEXEC, 3);
	if (gzfd < 0)
		return -errno;

	gzf = gzdopen(gzfd, "rb"); /* takes ownership of the fd */
	if (gzf == NULL) {
		close(gzfd);
		return -errno;
	}

	for (;;) {
		int r;

		if (did == total) {
			void *tmp = realloc(p, total + READ_STEP);
			if (tmp == NULL) {
				err = -errno;
				goto error;
			}
			total += READ_STEP;
			p = tmp;
		}

		r = gzread(gzf, p + did, total - did);
		if (r == 0)
			break;
		else if (r < 0) {
			int gzerr;
			const char *gz_errmsg = gzerror(gzf, &gzerr);

			ERR(file->ctx, "gzip: %s\n", gz_errmsg);

			/* gzip might not set errno here */
			err = gzerr == Z_ERRNO ? -errno : -EINVAL;
			goto error;
		}
		did += r;
	}

	file->memory = p;
	file->size = did;
	p = NULL;
	gzclose(gzf);
	return 0;

error:
	gzclose(gzf); /* closes the gzfd */
	return err;
}
