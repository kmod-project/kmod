// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2011-2013  ProFUSION embedded systems
 */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <shared/util.h>

#include "libkmod.h"
#include "libkmod-internal.h"
#include "libkmod-internal-file.h"

static const char magic_zstd[] = {0x28, 0xB5, 0x2F, 0xFD};
static const char magic_xz[] = {0xfd, '7', 'z', 'X', 'Z', 0};
static const char magic_zlib[] = {0x1f, 0x8b};

static int load_reg(struct kmod_file *file)
{
	struct stat st;

	if (fstat(file->fd, &st) < 0)
		return -errno;

	file->size = st.st_size;
	file->memory = mmap(NULL, file->size, PROT_READ, MAP_PRIVATE,
			    file->fd, 0);
	if (file->memory == MAP_FAILED) {
		file->memory = NULL;
		return -errno;
	}

	return 0;
}

static const struct comp_type {
	size_t magic_size;
	enum kmod_file_compression_type compression;
	const char *magic_bytes;
	int (*load)(struct kmod_file *file);
} comp_types[] = {
	// clang-format off
	{ sizeof(magic_zstd), KMOD_FILE_COMPRESSION_ZSTD, magic_zstd, kmod_file_load_zstd },
	{ sizeof(magic_xz), KMOD_FILE_COMPRESSION_XZ, magic_xz, kmod_file_load_xz },
	{ sizeof(magic_zlib), KMOD_FILE_COMPRESSION_ZLIB, magic_zlib, kmod_file_load_zlib },
	{ 0, KMOD_FILE_COMPRESSION_NONE, NULL, load_reg },
	// clang-format on
};

struct kmod_elf *kmod_file_get_elf(struct kmod_file *file)
{
	int err;

	if (file->elf)
		return file->elf;

	err = kmod_file_load_contents(file);
	if (err) {
		errno = err;
		return NULL;
	}

	file->elf = kmod_elf_new(file->memory, file->size);
	return file->elf;
}

struct kmod_file *kmod_file_open(const struct kmod_ctx *ctx,
						const char *filename)
{
	struct kmod_file *file;
	char buf[7];
	ssize_t sz;

	assert_cc(sizeof(magic_zstd) < sizeof(buf));
	assert_cc(sizeof(magic_xz) < sizeof(buf));
	assert_cc(sizeof(magic_zlib) < sizeof(buf));

	file = calloc(1, sizeof(struct kmod_file));
	if (file == NULL)
		return NULL;

	file->fd = open(filename, O_RDONLY|O_CLOEXEC);
	if (file->fd < 0) {
		free(file);
		return NULL;
	}

	sz = read_str_safe(file->fd, buf, sizeof(buf));
	lseek(file->fd, 0, SEEK_SET);
	if (sz != (sizeof(buf) - 1)) {
		if (sz < 0)
			errno = -sz;
		else
			errno = EINVAL;

		close(file->fd);
		free(file);
		return NULL;
	}

	for (unsigned int i = 0; i < ARRAY_SIZE(comp_types); i++) {
		const struct comp_type *itr = &comp_types[i];

		file->load = itr->load;
		file->compression = itr->compression;
		if (itr->magic_size &&
		    memcmp(buf, itr->magic_bytes, itr->magic_size) == 0) {
			break;
		}
	}

	file->ctx = ctx;

	return file;
}

/*
 *  Callers should just check file->memory got updated
 */
int kmod_file_load_contents(struct kmod_file *file)
{
	if (file->memory)
		return 0;

	/*  The load functions already log possible errors. */
	return file->load(file);
}

void *kmod_file_get_contents(const struct kmod_file *file)
{
	return file->memory;
}

off_t kmod_file_get_size(const struct kmod_file *file)
{
	return file->size;
}

enum kmod_file_compression_type kmod_file_get_compression(const struct kmod_file *file)
{
	return file->compression;
}

int kmod_file_get_fd(const struct kmod_file *file)
{
	return file->fd;
}

void kmod_file_unref(struct kmod_file *file)
{
	if (file->elf)
		kmod_elf_unref(file->elf);

	if (file->compression == KMOD_FILE_COMPRESSION_NONE) {
		if (file->memory)
			munmap(file->memory, file->size);
	} else {
		free(file->memory);
	}

	close(file->fd);
	free(file);
}
