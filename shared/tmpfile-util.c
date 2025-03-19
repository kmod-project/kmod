
// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2025  Intel Corporation.
 */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "tmpfile-util.h"

int read_link(int fd, char **ret_path)
{
	char proc_path[PATH_MAX];
	char fd_path[PATH_MAX];
	ssize_t len;

	len = snprintf(proc_path, sizeof(proc_path), "/proc/self/fd/%d", fd);
	if (len < 0 || len >= (ssize_t)sizeof(proc_path)) {
		return -errno;
	}

	len = readlink(proc_path, fd_path, sizeof(fd_path) - 1);
	if (len < 0 || len >= (ssize_t)sizeof(fd_path)) {
		return -errno;
	}

	fd_path[len] = '\0';

	if (ret_path != NULL) {
		*ret_path = strdup(fd_path);
	}

	return 0;
}

int tmpfile_init(struct tmpfile *file, const char *targetname)
{
	int fd;
	int mode = 0644;
	const char *pattern = "/tmp/tmpfileXXXXXXXX";

	if (file == NULL) {
		return -EINVAL;
	}

	file->targetname = targetname;
	memcpy(file->tmpname, pattern, strlen(pattern));

	fd = mkstemp(file->tmpname);
	if (fd < 0)
		goto create_fail;

	file->f = fopen(file->tmpname, "wb");
	if (file->f == NULL)
		goto create_fail;
	if (chmod(file->tmpname, mode) != 0)
		goto create_fail;

	return 0;
create_fail:
	file->f = NULL;
	file->targetname = NULL;
	memset(file->tmpname, 0, PATH_MAX);
	return -errno;
}

int tmpfile_publish(struct tmpfile *file)
{
	if (file == NULL || file->f == NULL) {
		return -EINVAL;
	}
	fclose(file->f);

	if (file->targetname == NULL)
		return -EINVAL;

	if (rename(file->tmpname, file->targetname) != 0)
		return -errno;

	return 0;
}

int tmpfile_release(struct tmpfile *file)
{
	if (file == NULL || file->f == NULL) {
		return -EINVAL;
	}
	fclose(file->f);

	if (remove(file->tmpname) != 0)
		return -errno;

	return 0;
}

int tmpfile_write(struct tmpfile *file, char *bytes, size_t count)
{
	if (file == NULL || file->f == NULL) {
		return -EINVAL;
	}
	return fwrite(bytes, 1, count, file->f);
}

int tmpfile_printf(struct tmpfile *file, char *fmt, ...)
{
	if (file == NULL || file->f == NULL) {
		return -EINVAL;
	}

	va_list args;
	va_start(args, fmt);
	int result = vfprintf(file->f, fmt, args);
	va_end(args);

	return result;
}
