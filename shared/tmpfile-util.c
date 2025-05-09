// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2025  Intel Corporation.
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
#include <shared/tmpfile-util.h>
#include <shared/util.h>

FILE *tmpfile_openat(int dirfd, mode_t mode, struct tmpfile *file)
{
	const char *tmpname_tmpl = "tmpfileXXXXXX";
	const char *tmpname;
	char tmpfile_path[PATH_MAX];
	int fd, n;
	_cleanup_free_ char *targetdir;
	FILE *fp;

	targetdir = fd_lookup_path(dirfd);
	if (targetdir == NULL)
		goto create_fail;

	n = snprintf(tmpfile_path, sizeof(tmpfile_path), "%s/%s", targetdir, tmpname_tmpl);
	if (n < 0 || n >= (int)sizeof(tmpfile_path))
		goto create_fail;

	fd = mkstemp(tmpfile_path);
	if (fd < 0)
		goto create_fail;

	if (fchmod(fd, mode) < 0)
		goto checkout_fail;

	fp = fdopen(fd, "wb");
	if (fp == NULL)
		goto checkout_fail;

	tmpname = basename(tmpfile_path);
	memcpy(file->tmpname, tmpname, strlen(tmpname) + 1);

	file->dirfd = dirfd;
	file->fd = fd;

	return fp;

checkout_fail:
	close(fd);
	remove(tmpfile_path);

create_fail:
	return NULL;
}

int tmpfile_publish(struct tmpfile *file, const char *targetname)
{
	if (renameat(file->dirfd, file->tmpname, file->dirfd, targetname) != 0)
		return -errno;

	file->fd = -1;
	file->dirfd = -1;
	memset(file->tmpname, 0, PATH_MAX);

	return 0;
}

void tmpfile_release(struct tmpfile *file)
{
	unlinkat(file->dirfd, file->tmpname, 0);

	file->fd = -1;
	file->dirfd = -1;
	file->tmpname[0] = '\0';
}
