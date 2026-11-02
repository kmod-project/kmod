// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2025 Intel Corporation.
 */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <shared/macro.h>
#include <shared/missing.h>
#include <shared/tmpfile.h>
#include <shared/util.h>

int tmpfile_openat(int dirfd, mode_t mode, struct tmpfile *file)
{
	const char *tmpname_tmpl = "/tmpfileXXXXXX";
	const char *tmpname;
	char tmpfile_path[PATH_MAX];
	int fd, n, ret;
	FILE *fp;

	n = fd_lookup_path(dirfd, tmpfile_path, sizeof(tmpfile_path));
	if (n < 0) {
		ret = n;
		goto create_fail;
	}

	if (n + strlen(tmpname_tmpl) + 1 > sizeof(tmpfile_path)) {
		ret = ENOMEM;
		goto create_fail;
	}

	memcpy(tmpfile_path + n, tmpname_tmpl, strlen(tmpname_tmpl) + 1);

	fd = mkstemp(tmpfile_path);
	if (fd < 0) {
		ret = -errno;
		goto create_fail;
	}

	if (fchmod(fd, mode) < 0) {
		ret = -errno;
		goto checkout_fail;
	}

	fp = fdopen(fd, "wb");
	if (fp == NULL) {
		ret = -errno;
		goto checkout_fail;
	}

	tmpname = basename(tmpfile_path);
	memcpy(file->tmpname, tmpname, strlen(tmpname) + 1);

	file->dirfd = dirfd;
	file->fp = fp;

	return 0;

checkout_fail:
	close(fd);
	remove(tmpfile_path);

create_fail:
	return ret;
}

static void tmpfile_cleanup(struct tmpfile *file)
{
	file->dirfd = -1;
	file->fp = NULL;
	memset(file->tmpname, 0, sizeof(file->tmpname));
}

int tmpfile_publish(struct tmpfile *file, const char *targetname)
{
	if (renameat(file->dirfd, file->tmpname, file->dirfd, targetname) != 0)
		return -errno;

	tmpfile_cleanup(file);
	return 0;
}

void tmpfile_release(struct tmpfile *file)
{
	unlinkat(file->dirfd, file->tmpname, 0);
	tmpfile_cleanup(file);
}
