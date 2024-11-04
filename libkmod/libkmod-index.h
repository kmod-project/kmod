/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2011-2013  ProFUSION embedded systems
 */

#pragma once

#include <inttypes.h>

struct index_value {
	struct index_value *next;
	uint32_t priority;
	size_t len;
	char value[0];
};

/* In-memory index (depmod only) */
struct index_file;
struct index_file *index_file_open(const char *filename);
void index_file_close(struct index_file *idx);
char *index_search(struct index_file *idx, const char *key);
void index_dump(struct index_file *in, int fd, bool alias_prefix);
struct index_value *index_searchwild(struct index_file *idx, const char *key);

void index_values_free(struct index_value *values);

/* Implementation using mmap */
struct index_mm;
int index_mm_open(const struct kmod_ctx *ctx, const char *filename,
		  unsigned long long *stamp, struct index_mm **pidx);
void index_mm_close(struct index_mm *index);
char *index_mm_search(const struct index_mm *idx, const char *key);
struct index_value *index_mm_searchwild(const struct index_mm *idx, const char *key);
void index_mm_dump(const struct index_mm *idx, int fd, bool alias_prefix);
