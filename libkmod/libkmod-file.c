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

#include <stdbool.h>
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

#ifdef ENABLE_XZ
#include <lzma.h>
#endif
#ifdef ENABLE_ZLIB
#include <zlib.h>
#endif

struct kmod_file {
#ifdef ENABLE_XZ
	bool xz_used;
#endif
#ifdef ENABLE_ZLIB
	gzFile gzf;
#endif
	int fd;
	off_t size;
	void *memory;
};

#ifdef ENABLE_XZ
static bool xz_detect(struct kmod_file *file)
{
	ssize_t ret;
	char buf[6];

	ret = read(file->fd, buf, sizeof(buf));
	if (ret < 0 || lseek(file->fd, 0, SEEK_SET))
		return -errno;
	return ret == sizeof(buf) &&
	       memcmp(buf, "\xFD""7zXZ\x00", sizeof(buf)) == 0;
}

static void xz_uncompress_belch(lzma_ret ret)
{
	switch (ret) {
	case LZMA_MEM_ERROR:
		fprintf(stderr, "xz: %s\n", strerror(ENOMEM));
		break;
	case LZMA_FORMAT_ERROR:
		fprintf(stderr, "xz: File format not recognized\n");
		break;
	case LZMA_OPTIONS_ERROR:
		fprintf(stderr, "xz: Unsupported compression options\n");
		break;
	case LZMA_DATA_ERROR:
		fprintf(stderr, "xz: File is corrupt\n");
		break;
	case LZMA_BUF_ERROR:
		fprintf(stderr, "xz: Unexpected end of input\n");
		break;
	default:
		fprintf(stderr, "xz: Internal error (bug)\n");
		break;
	}
}

static int xz_uncompress(lzma_stream *strm, struct kmod_file *file)
{
	uint8_t in_buf[BUFSIZ], out_buf[BUFSIZ];
	lzma_action action = LZMA_RUN;
	lzma_ret ret;
	void *p = NULL;
	size_t total = 0;

	strm->avail_in  = 0;
	strm->next_out  = out_buf;
	strm->avail_out = sizeof(out_buf);

	while (true) {
		if (strm->avail_in == 0) {
			ssize_t rdret = read(file->fd, in_buf, sizeof(in_buf));
			if (rdret < 0) {
				ret = -errno;
				goto out;
			}
			strm->next_in  = in_buf;
			strm->avail_in = rdret;
			if (rdret == 0)
				action = LZMA_FINISH;
		}
		ret = lzma_code(strm, action);
		if (strm->avail_out == 0 || ret != LZMA_OK) {
			size_t write_size = BUFSIZ - strm->avail_out;
			char *tmp = realloc(p, total + write_size);
			if (tmp == NULL) {
				ret = -errno;
				goto out;
			}
			memcpy(tmp + total, out_buf, write_size);
			total += write_size;
			p = tmp;
			strm->next_out = out_buf;
			strm->avail_out = BUFSIZ;
		}
		if (ret == LZMA_STREAM_END)
			break;
		if (ret != LZMA_OK) {
			xz_uncompress_belch(ret);
			ret = -EINVAL;
			goto out;
		}
	}
	file->memory = p;
	file->size = total;
	return 0;
 out:
	free(p);
	close(file->fd);
	return ret;
}

static int xz_file_open(struct kmod_file *file)
{
	lzma_stream strm = LZMA_STREAM_INIT;
	lzma_ret lzret;
	int ret;

	lzret = lzma_stream_decoder(&strm, UINT64_MAX, LZMA_CONCATENATED);
	if (lzret == LZMA_MEM_ERROR) {
		fprintf(stderr, "xz: %s\n", strerror(ENOMEM));
		close(file->fd);
		return -ENOMEM;
	} else if (lzret != LZMA_OK) {
		fprintf(stderr, "xz: Internal error (bug)\n");
		close(file->fd);
		return -EINVAL;
	}
	ret = xz_uncompress(&strm, file);
	lzma_end(&strm);
	return ret;
}
#endif

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

#ifdef ENABLE_XZ
	if (xz_detect(file))
		err = xz_file_open(file);
	else
#endif
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
#ifdef ENABLE_XZ
	if (file->xz_used) {
		free(file->memory);
		close(file->fd);
	} else
#endif
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
