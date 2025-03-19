#pragma once

#include <stdio.h>
#include <linux/limits.h>

struct tmpfile {
	char tmpname[PATH_MAX];
	int dirfd;
	int fd;
};

/*
 * Create a temporary file at the directory of `dirfd`
*/
FILE *tmpfile_openat(int dirfd, const char *tmpname_tmpl, mode_t mode,
		     struct tmpfile *file);

/*
 * Move the temporary file to `targetname`
*/
int tmpfile_publish(struct tmpfile *file, const char *targetname);

/*
 * Delete the temporary file
*/
void tmpfile_release(struct tmpfile *file);
