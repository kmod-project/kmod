// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright © 2025 Intel Corporation
 */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <shared/tmpfile-util.h>
#include <shared/util.h>

static int read_link(int fd, char **ret_path)
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

int fopen_temporary_at(int dir_fd, int *ret_fd, char **ret_path)
{
	char tmp_path[PATH_MAX];
	_cleanup_free_ char *dir_path = NULL;
	int mode = 0644;
	int n;
	int fd;
	int err;

	// Get the directory path
	err = read_link(dir_fd, &dir_path);
	if (err < 0 || dir_path == NULL) {
		return -EINVAL;
	}

	n = snprintf(tmp_path, sizeof(tmp_path), "%s/tmpfileXXXXXX", dir_path);
	if (n > (int)sizeof(tmp_path) || n < 0) {
		return -EINVAL;
	}

	fd = mkstemp(tmp_path);
	if (fd < 0) {
		return -errno;
	}

	if (fchmod(fd, mode) < 0) {
		close(fd);
		return -errno;
	}

	*ret_fd = fd;
	if (ret_path)
		*ret_path = strdup(tmp_path);

	return 0;
}
