// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright © 2024 Intel Corporation
 */

#define DLSYM_LOCALLY_ENABLED ENABLE_XZ_DLOPEN

#include <errno.h>
#include <lzma.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <shared/elf-note.h>
#include <shared/util.h>

#include "libkmod.h"
#include "libkmod-internal.h"
#include "libkmod-internal-file.h"

#define DL_SYMBOL_TABLE(M)     \
	M(lzma_stream_decoder) \
	M(lzma_code)           \
	M(lzma_end)

DL_SYMBOL_TABLE(DECLARE_SYM)

static int dlopen_lzma(void)
{
#if !DLSYM_LOCALLY_ENABLED
	return 0;
#else
	static void *dl;

	ELF_NOTE_DLOPEN("xz", "Support for uncompressing xz-compressed modules",
			ELF_NOTE_DLOPEN_PRIORITY_RECOMMENDED, "liblzma.so.5");

	return dlsym_many(&dl, "liblzma.so.5", DL_SYMBOL_TABLE(DLSYM_ARG) NULL);
#endif
}

static void xz_uncompress_belch(struct kmod_file *file, lzma_ret ret)
{
	switch (ret) {
	case LZMA_MEM_ERROR:
		ERR(file->ctx, "xz: %s\n", strerror(ENOMEM));
		break;
	case LZMA_FORMAT_ERROR:
		ERR(file->ctx, "xz: File format not recognized\n");
		break;
	case LZMA_OPTIONS_ERROR:
		ERR(file->ctx, "xz: Unsupported compression options\n");
		break;
	case LZMA_DATA_ERROR:
		ERR(file->ctx, "xz: File is corrupt\n");
		break;
	case LZMA_BUF_ERROR:
		ERR(file->ctx, "xz: Unexpected end of input\n");
		break;
	default:
		ERR(file->ctx, "xz: Internal error (bug)\n");
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

	strm->avail_in = 0;
	strm->next_out = out_buf;
	strm->avail_out = sizeof(out_buf);

	while (true) {
		if (strm->avail_in == 0) {
			ssize_t rdret = read(file->fd, in_buf, sizeof(in_buf));
			if (rdret < 0) {
				ret = -errno;
				goto out;
			}
			strm->next_in = in_buf;
			strm->avail_in = rdret;
			if (rdret == 0)
				action = LZMA_FINISH;
		}
		ret = sym_lzma_code(strm, action);
		if (strm->avail_out == 0 || ret != LZMA_OK) {
			size_t write_size = BUFSIZ - strm->avail_out;
			char *tmp = realloc(p, total + write_size);
			if (tmp == NULL) {
				ret = -ENOMEM;
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
			xz_uncompress_belch(file, ret);
			ret = -EINVAL;
			goto out;
		}
	}
	file->memory = p;
	file->size = total;
	return 0;
out:
	free(p);
	return ret;
}

int kmod_file_load_xz(struct kmod_file *file)
{
	lzma_stream strm = LZMA_STREAM_INIT;
	lzma_ret lzret;
	int ret;

	ret = dlopen_lzma();
	if (ret < 0) {
		ERR(file->ctx, "xz: can't load and resolve symbols (%s)", strerror(-ret));
		return -EINVAL;
	}

	lzret = sym_lzma_stream_decoder(&strm, UINT64_MAX, LZMA_CONCATENATED);
	if (lzret == LZMA_MEM_ERROR) {
		ERR(file->ctx, "xz: %s\n", strerror(ENOMEM));
		return -ENOMEM;
	} else if (lzret != LZMA_OK) {
		ERR(file->ctx, "xz: Internal error (bug)\n");
		return -EINVAL;
	}
	ret = xz_uncompress(&strm, file);
	sym_lzma_end(&strm);
	return ret;
}
