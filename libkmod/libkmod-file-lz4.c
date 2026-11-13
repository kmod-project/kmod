// SPDX-License-Identifier: LGPL-2.1-or-later

#define DLSYM_LOCALLY_ENABLED ENABLE_LZ4_DLOPEN

#include <errno.h>
#include <lz4frame.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <shared/elf-note.h>
#include <shared/util.h>

#include "libkmod.h"
#include "libkmod-internal.h"
#include "libkmod-internal-file.h"

#define DL_SYMBOL_TABLE(M)                     \
	M(LZ4F_createDecompressionContext)     \
	M(LZ4F_freeDecompressionContext)       \
	M(LZ4F_getFrameInfo)                   \
	M(LZ4F_decompress)                     \
	M(LZ4F_isError)                        \
	M(LZ4F_getErrorName)

DL_SYMBOL_TABLE(DECLARE_SYM)

static int dlopen_lz4(void)
{
#if !DLSYM_LOCALLY_ENABLED
	return 0;
#else
	static void *dl;

	ELF_NOTE_DLOPEN("lz4", "Support for uncompressing lz4-compressed modules",
			ELF_NOTE_DLOPEN_PRIORITY_RECOMMENDED, "liblz4.so.1");

	return dlsym_many(&dl, "liblz4.so.1", DL_SYMBOL_TABLE(DLSYM_ARG) NULL);
#endif
}

int kmod_file_load_lz4(struct kmod_file *file)
{
	LZ4F_dctx *dctx = NULL;
	LZ4F_frameInfo_t info = { 0 };
	void *src_buf = MAP_FAILED;
	_cleanup_free_ uint8_t *dst_buf = NULL;
	size_t src_size, src_pos, dst_size, dst_pos = 0, hint;
	struct stat st;
	int ret;

	ret = dlopen_lz4();
	if (ret < 0) {
		ERR(file->ctx, "lz4: can't load and resolve symbols (%s)\n",
		    strerror(-ret));
		return -EINVAL;
	}

	if (fstat(file->fd, &st) < 0) {
		ret = -errno;
		ERR(file->ctx, "lz4: %m\n");
		return ret;
	}

	if ((uintmax_t)st.st_size > SIZE_MAX)
		return -ENOMEM;

	src_size = st.st_size;
	src_buf = mmap(NULL, src_size, PROT_READ, MAP_PRIVATE, file->fd, 0);
	if (src_buf == MAP_FAILED)
		return -errno;

	hint = sym_LZ4F_createDecompressionContext(&dctx, LZ4F_VERSION);
	if (sym_LZ4F_isError(hint)) {
		ret = -ENOMEM;
		goto out;
	}

	src_pos = src_size;
	hint = sym_LZ4F_getFrameInfo(dctx, &info, src_buf, &src_pos);
	if (sym_LZ4F_isError(hint)) {
		ERR(file->ctx, "lz4: %s\n", sym_LZ4F_getErrorName(hint));
		ret = -EINVAL;
		goto out;
	}

	if (info.contentSize > SIZE_MAX - 1) {
		ret = -ENOMEM;
		goto out;
	}
	/* One spare byte to notice content beyond the declared size. */
	dst_size = info.contentSize ? info.contentSize + 1 : 4 * src_size;
	if (dst_size < 65536)
		dst_size = 65536;

	while (hint != 0) {
		size_t in_len = src_size - src_pos;
		size_t out_len;

		if (dst_pos == dst_size) {
			uint8_t *tmp;

			if (dst_size > SIZE_MAX / 2) {
				ret = -ENOMEM;
				goto out;
			}
			tmp = realloc(dst_buf, dst_size * 2);
			if (tmp == NULL) {
				ret = -ENOMEM;
				goto out;
			}
			dst_buf = tmp;
			dst_size *= 2;
		} else if (dst_buf == NULL) {
			dst_buf = malloc(dst_size);
			if (dst_buf == NULL) {
				ret = -ENOMEM;
				goto out;
			}
		}

		out_len = dst_size - dst_pos;
		hint = sym_LZ4F_decompress(dctx, dst_buf + dst_pos, &out_len,
					   (uint8_t *)src_buf + src_pos, &in_len, NULL);
		if (sym_LZ4F_isError(hint)) {
			ERR(file->ctx, "lz4: %s\n", sym_LZ4F_getErrorName(hint));
			ret = -EINVAL;
			goto out;
		}

		src_pos += in_len;
		dst_pos += out_len;

		if (hint != 0 && in_len == 0 && out_len == 0) {
			ERR(file->ctx, "lz4: Unexpected end of input\n");
			ret = -EINVAL;
			goto out;
		}
	}

	if (src_pos != src_size) {
		ERR(file->ctx, "lz4: Trailing data after the frame\n");
		ret = -EINVAL;
		goto out;
	}

	if (info.contentSize && dst_pos != info.contentSize) {
		ERR(file->ctx, "lz4: Content size mismatch\n");
		ret = -EINVAL;
		goto out;
	}

	file->memory = TAKE_PTR(dst_buf);
	file->size = dst_pos;
	ret = 0;

out:
	if (dctx)
		sym_LZ4F_freeDecompressionContext(dctx);
	munmap(src_buf, src_size);
	return ret;
}
