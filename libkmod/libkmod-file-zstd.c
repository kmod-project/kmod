// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright © 2024 Intel Corporation
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <zstd.h>

#include <shared/util.h>

#include "libkmod.h"
#include "libkmod-internal.h"
#include "libkmod-internal-file.h"

static int zstd_read_block(struct kmod_file *file, size_t block_size,
			   ZSTD_inBuffer *input, size_t *input_capacity)
{
	ssize_t rdret;
	int ret;

	if (*input_capacity < block_size) {
		free((void *)input->src);
		input->src = malloc(block_size);
		if (input->src == NULL) {
			ret = -errno;
			ERR(file->ctx, "zstd: %m\n");
			return ret;
		}
		*input_capacity = block_size;
	}

	rdret = read(file->fd, (void *)input->src, block_size);
	if (rdret < 0) {
		ret = -errno;
		ERR(file->ctx, "zstd: %m\n");
		return ret;
	}

	input->pos = 0;
	input->size = rdret;
	return 0;
}

static int zstd_ensure_outbuffer_space(ZSTD_outBuffer *buffer, size_t min_free)
{
	uint8_t *old_buffer = buffer->dst;
	int ret = 0;

	if (buffer->size - buffer->pos >= min_free)
		return 0;

	if (buffer->size < min_free)
		buffer->size = min_free;
	else
		buffer->size *= 2;

	buffer->size += min_free;
	buffer->dst = realloc(buffer->dst, buffer->size);
	if (buffer->dst == NULL) {
		ret = -errno;
		free(old_buffer);
	}

	return ret;
}

static int zstd_decompress_block(struct kmod_file *file, ZSTD_DStream *dstr,
				 ZSTD_inBuffer *input, ZSTD_outBuffer *output,
				 size_t *next_block_size)
{
	size_t out_buf_min_size = ZSTD_DStreamOutSize();
	int ret = 0;

	do {
		ssize_t dsret;

		ret = zstd_ensure_outbuffer_space(output, out_buf_min_size);
		if (ret) {
			ERR(file->ctx, "zstd: %s\n", strerror(-ret));
			break;
		}

		dsret = ZSTD_decompressStream(dstr, output, input);
		if (ZSTD_isError(dsret)) {
			ret = -EINVAL;
			ERR(file->ctx, "zstd: %s\n", ZSTD_getErrorName(dsret));
			break;
		}
		if (dsret > 0)
			*next_block_size = (size_t)dsret;
	} while (input->pos < input->size
		 || output->pos > output->size
		 || output->size - output->pos < out_buf_min_size);

	return ret;
}

int kmod_file_load_zstd(struct kmod_file *file)
{
	ZSTD_DStream *dstr;
	size_t next_block_size;
	size_t zst_inb_capacity = 0;
	ZSTD_inBuffer zst_inb = { 0 };
	ZSTD_outBuffer zst_outb = { 0 };
	int ret;

	dstr = ZSTD_createDStream();
	if (dstr == NULL) {
		ret = -EINVAL;
		ERR(file->ctx, "zstd: Failed to create decompression stream\n");
		goto out;
	}

	next_block_size = ZSTD_initDStream(dstr);

	while (true) {
		ret = zstd_read_block(file, next_block_size, &zst_inb,
				      &zst_inb_capacity);
		if (ret != 0)
			goto out;
		if (zst_inb.size == 0) /* EOF */
			break;

		ret = zstd_decompress_block(file, dstr, &zst_inb, &zst_outb,
					    &next_block_size);
		if (ret != 0)
			goto out;
	}

	ZSTD_freeDStream(dstr);
	free((void *)zst_inb.src);
	file->memory = zst_outb.dst;
	file->size = zst_outb.pos;
	return 0;
out:
	if (dstr != NULL)
		ZSTD_freeDStream(dstr);
	free((void *)zst_inb.src);
	free((void *)zst_outb.dst);
	return ret;
}
