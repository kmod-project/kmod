// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright © 2024 Intel Corporation
 */

#include <libkmod/libkmod-internal.h>

struct kmod_ctx;
struct kmod_elf;

struct kmod_file {
	int fd;
	enum kmod_file_compression_type compression;
	off_t size;
	void *memory;
	int (*load)(struct kmod_file *file);
	const struct kmod_ctx *ctx;
	struct kmod_elf *elf;
};

#if ENABLE_XZ
int kmod_file_load_xz(struct kmod_file *file);
#else
static inline int kmod_file_load_xz(_maybe_unused_ struct kmod_file *file)
{
	return -ENOSYS;
}
#endif

#if ENABLE_ZLIB
int kmod_file_load_zlib(struct kmod_file *file);
#else
static inline int kmod_file_load_zlib(_maybe_unused_ struct kmod_file *file)
{
	return -ENOSYS;
}
#endif

#if ENABLE_ZSTD
int kmod_file_load_zstd(struct kmod_file *file);
#else
static inline int kmod_file_load_zstd(_maybe_unused_ struct kmod_file *file)
{
	return -ENOSYS;
}
#endif
