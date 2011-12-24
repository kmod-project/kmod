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
#endif
	int fd;
	off_t size;
	void *memory;
};

#ifdef ENABLE_ZLIB
#define READ_STEP (4 * 1024 * 1024)

static bool check_zlib(struct kmod_file *file)
{
	uint8_t magic[2] = {0, 0}, zlibmagic[2] = {0x1f, 0x8b};
	size_t done = 0, todo = 2;
	while (todo > 0) {
		ssize_t r = read(file->fd, magic + done, todo);
		if (r > 0){
			todo -= r;
			done += r;
		} else if (r == 0)
			goto error;
		else if (errno == EAGAIN || errno == EINTR)
			continue;
		else
			goto error;
	}
	lseek(file->fd, 0, SEEK_SET);
	return memcmp(magic, zlibmagic, 2) == 0;
error:
	lseek(file->fd, 0, SEEK_SET);
	return false;
}

static int zlib_file_open(struct kmod_file *file)
{
	int err = 0;
	off_t did = 0, total = 0;
	unsigned char *p = NULL;

	errno = 0;
	file->gzf = gzdopen(file->fd, "rb");
	if (file->gzf == NULL) {
		close(file->fd);
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
#endif

static int reg_file_open(struct kmod_file *file)
{
	struct stat st;
	int err = 0;

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

struct kmod_file *kmod_file_open(const char *filename)
{
	struct kmod_file *file = calloc(1, sizeof(struct kmod_file));
	int err;

	if (file == NULL)
		return NULL;

	file->fd = open(filename, O_RDONLY|O_CLOEXEC);
	if (file->fd < 0) {
		err = -errno;
		goto error;
	}

#ifdef ENABLE_ZLIB
	if (check_zlib(file))
		err = zlib_file_open(file);
	else
#endif
		err = reg_file_open(file);

error:
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
	if (file->gzf != NULL) {
		free(file->memory);
		gzclose(file->gzf);
	} else {
#endif
		munmap(file->memory, file->size);
		close(file->fd);
#ifdef ENABLE_ZLIB
	}
#endif
	free(file);
}
