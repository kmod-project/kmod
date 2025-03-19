
// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2025  Intel Corporation.
 */

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "tmpfile-util.h"
#include "macro.h"

int tmpfile_init(struct tmpfile *file, const char *targetname)
{
	int fd, n, err;
	int mode = 0644;
	char *targetdir;
	_cleanup_free_ char *dup_target = strdup(targetname);

	if (file == NULL || targetname == NULL) {
		return -EINVAL;
	}

	targetdir = dirname(dup_target);
	memset(file->tmpname, 0, PATH_MAX);
	n = snprintf(file->tmpname, PATH_MAX, "%s/tmpfileXXXXXXXX", targetdir);
	if (n >= PATH_MAX) {
		err = -EINVAL;
		goto create_fail;
	}

	file->targetname = targetname;

	fd = mkstemp(file->tmpname);
	if (fd < 0) {
		err = -errno;
		goto create_fail;
	}

	if (fchmod(fd, mode) < 0) {
		close(fd);
		err = -errno;
		goto create_fail;
	}

	file->f = fdopen(fd, "wb");
	if (file->f == NULL) {
		close(fd);
		err = -errno;
		goto create_fail;
	}

	return 0;
create_fail:
	file->f = NULL;
	file->targetname = NULL;
	memset(file->tmpname, 0, PATH_MAX);
	return err;
}

int tmpfile_publish(struct tmpfile *file)
{
	if (file == NULL || file->f == NULL) {
		return -EINVAL;
	}
	fclose(file->f);

	if (file->targetname == NULL)
		return -EINVAL;

	printf("from: %s, to: %s\n", file->tmpname, file->targetname);
	if (rename(file->tmpname, file->targetname) != 0)
		return -errno;

	file->f = NULL;
	file->targetname = NULL;
	memset(file->tmpname, 0, PATH_MAX);
	return 0;
}

void tmpfile_release(struct tmpfile *file)
{
	if (file == NULL || file->f == NULL) {
		return;
	}
	fclose(file->f);

	if (access(file->tmpname, F_OK) == 0) {
		remove(file->tmpname);
	}

	file->f = NULL;
	file->targetname = NULL;
	memset(file->tmpname, 0, PATH_MAX);
}

int tmpfile_write(struct tmpfile *file, const char *bytes, size_t count)
{
	if (file == NULL || file->f == NULL) {
		return -EINVAL;
	}
	return fwrite(bytes, 1, count, file->f);
}

int tmpfile_printf(struct tmpfile *file, const char *fmt, ...)
{
	va_list args;
	int result;
	if (file == NULL || file->f == NULL) {
		return -EINVAL;
	}

	va_start(args, fmt);
	result = vfprintf(file->f, fmt, args);
	va_end(args);

	return result;
}
