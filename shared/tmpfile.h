// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2025  Intel Corporation.
 */

#pragma once

#include <stdio.h>
#include <linux/limits.h>

#include <shared/macro.h>

struct tmpfile {
	char tmpname[PATH_MAX];
	int dirfd;
	int fd;
};

/*
 * Create a temporary file at the directory of `dirfd`
*/
_must_check_ _nonnull_(3) FILE *tmpfile_openat(int dirfd, mode_t mode,
					       struct tmpfile *file);

/*
 * Move the temporary file to `targetname`
*/
_nonnull_all_ int tmpfile_publish(struct tmpfile *file, const char *targetname);

/*
 * Delete the temporary file
*/
_nonnull_all_ void tmpfile_release(struct tmpfile *file);
