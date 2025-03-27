#pragma once

#include <stdio.h>
#include <linux/limits.h>

struct tmpfile {
	char tmpname[PATH_MAX];
	const char *targetname;
	FILE *f;
};

/*
 * Create a temporary file at the same directory of `targename`, the temp file name
 * follows pattern tmpfileXXXXXXXX. After creation, open the temporary file to `FILE *f`
 * and change the file permissions to 0644.
*/
int tmpfile_init(struct tmpfile *file, const char *targetname);

/*
 * Move the temporary file to `file->targetname`
*/
int tmpfile_publish(struct tmpfile *file);

/*
 * Delete the temporary file
*/
void tmpfile_release(struct tmpfile *file);

int tmpfile_write(struct tmpfile *file, const char *bytes, size_t count);

int tmpfile_printf(struct tmpfile *file, const char *fmt, ...)
	__attribute__((format(printf, 2, 3)));
