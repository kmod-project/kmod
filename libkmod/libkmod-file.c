/*
 * libkmod - interface to kernel module operations
 *
 * Copyright (C) 2011  ProFUSION embedded systems
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

#include "libkmod.h"
#include "libkmod-private.h"

#ifdef ENABLE_ZLIB
#include <zlib.h>
#endif

struct kmod_file {
#ifdef ENABLE_ZLIB
	gzFile gzf;
#else
	int fd;
#endif
	off_t size;
	void *memory;
};

#ifdef ENABLE_ZLIB
#define READ_STEP (4 * 1024 * 1024)
static int zlip_file_open(struct kmod_file *file, const char *filename)
{
	int err = 0;
	off_t did = 0, total = 0;
	unsigned char *p = NULL;

	errno = 0;
	file->gzf = gzopen(filename, "rbe");
	if (file->gzf == NULL)
		return -errno;

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

		r = gzread(file->gzf, p + did, total - did);
		if (r == 0)
			break;
		else if (r < 0) {
			err = -errno;
			goto error;
		}
		did += r;
	}

	file->memory = p;
	file->size = did;
	return 0;
error:
	free(p);
	gzclose(file->gzf);
	return err;
}
#else
static int reg_file_open(struct kmod_file *file, const char *filename)
{
	struct stat st;
	int err = 0;

	file->fd = open(filename, O_RDONLY|O_CLOEXEC);
	if (file->fd < 0)
		return -errno;

	if (fstat(file->fd, &st) < 0) {
		err = -errno;
		goto error;
	}

	file->size = st.st_size;
	file->memory = mmap(0, file->size, PROT_READ, MAP_PRIVATE, file->fd, 0);
	if (file->memory == MAP_FAILED) {
		err = -errno;
		goto error;
	}

	return 0;
error:
	close(file->fd);
	return err;
}
#endif

struct kmod_file *kmod_file_open(const char *filename)
{
	struct kmod_file *file = calloc(1, sizeof(struct kmod_file));
	int err;

	if (file == NULL)
		return NULL;

#ifdef ENABLE_ZLIB
	err = zlip_file_open(file, filename);
#else
	err = reg_file_open(file, filename);
#endif

	if (err < 0) {
		free(file);
		errno = -err;
		return NULL;
	}

	return file;
}

void *kmod_file_get_contents(const struct kmod_file *file)
{
	return file->memory;
}

off_t kmod_file_get_size(const struct kmod_file *file)
{
	return file->size;
}

void kmod_file_unref(struct kmod_file *file)
{
#ifdef ENABLE_ZLIB
	free(file->memory);
	gzclose(file->gzf);
#else
	munmap(file->memory, file->size);
	close(file->fd);
#endif
	free(file);
}
