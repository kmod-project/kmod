// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright © 2024 Intel Corporation
 */

#define DLSYM_LOCALLY_ENABLED ENABLE_ZSTD_DLOPEN

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <zstd.h>

#include <shared/elf-note.h>
#include <shared/util.h>

#include "libkmod.h"
#include "libkmod-internal.h"
#include "libkmod-internal-file.h"

#define DL_SYMBOL_TABLE(M)          \
	M(ZSTD_decompress)          \
	M(ZSTD_getErrorName)        \
	M(ZSTD_getFrameContentSize) \
	M(ZSTD_isError)

DL_SYMBOL_TABLE(DECLARE_SYM)

static int dlopen_zstd(void)
{
#if !DLSYM_LOCALLY_ENABLED
	return 0;
#else
	static void *dl;

	ELF_NOTE_DLOPEN("zstd", "Support for uncompressing zstd-compressed modules",
			ELF_NOTE_DLOPEN_PRIORITY_RECOMMENDED, "libzstd.so.1");

	return dlsym_many(&dl, "libzstd.so.1", DL_SYMBOL_TABLE(DLSYM_ARG) NULL);
#endif
}

int kmod_file_load_zstd(struct kmod_file *file)
{
	void *src_buf = MAP_FAILED, *dst_buf = NULL;
	size_t src_size, dst_size;
	unsigned long long frame_size;
	struct stat st;
	int ret;

	ret = dlopen_zstd();
	if (ret < 0) {
		ERR(file->ctx, "zstd: can't load and resolve symbols (%s)",
		    strerror(-ret));
		return -EINVAL;
	}

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

	frame_size = sym_ZSTD_getFrameContentSize(src_buf, src_size);
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
		ret = -ENOMEM;
		goto out;
	}

	dst_size = sym_ZSTD_decompress(dst_buf, dst_size, src_buf, src_size);
	if (sym_ZSTD_isError(dst_size)) {
		ERR(file->ctx, "zstd: %s\n", sym_ZSTD_getErrorName(dst_size));
		ret = -EINVAL;
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
