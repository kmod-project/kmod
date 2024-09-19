// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright © 2024 Intel Corporation
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <zstd.h>

#include <shared/util.h>

#include "libkmod.h"
#include "libkmod-internal.h"
#include "libkmod-internal-file.h"

int kmod_file_load_zstd(struct kmod_file *file)
{
	void *src_buf = MAP_FAILED, *dst_buf = NULL;
	size_t src_size, dst_size, ret;
	unsigned long long frame_size;
	struct stat st;

	if (fstat(file->fd, &st) < 0) {
		ret = -errno;
		ERR(file->ctx, "zstd: %m\n");
		goto out;
	}

	if ((uintmax_t)st.st_size > SIZE_MAX) {
		ret = -ENOMEM;
		goto out;
	}

	src_size = st.st_size;
	src_buf = mmap(NULL, src_size, PROT_READ, MAP_PRIVATE, file->fd, 0);
	if (src_buf == MAP_FAILED) {
		ret = -errno;
		goto out;
	}

	frame_size = ZSTD_getFrameContentSize(src_buf, src_size);
	if (frame_size == 0 || frame_size == ZSTD_CONTENTSIZE_UNKNOWN ||
	    frame_size == ZSTD_CONTENTSIZE_ERROR) {
		ret = -EINVAL;
		ERR(file->ctx, "zstd: Failed to determine decompression size\n");
		goto out;
	}

	if (frame_size > SIZE_MAX) {
		ret = -ENOMEM;
		goto out;
	}

	dst_size = frame_size;
	dst_buf = malloc(dst_size);
	if (dst_buf == NULL) {
		ret = -errno;
		goto out;
	}

	ret = ZSTD_decompress(dst_buf, dst_size, src_buf, src_size);
	if (ZSTD_isError(ret)) {
		ERR(file->ctx, "zstd: %s\n", ZSTD_getErrorName(ret));
		goto out;
	}

	file->memory = dst_buf;
	file->size = dst_size;

	ret = 0;
	dst_buf = NULL;

out:
	free(dst_buf);

	if (src_buf != MAP_FAILED)
		munmap(src_buf, src_size);

	return ret;
}
