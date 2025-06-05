// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2019  Alexey Gladkov <gladkov.alexey@gmail.com>
 * Copyright (C) 2024  Tobias Stöckmann <tobias@stoeckmann.org>
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <shared/strbuf.h>
#include <shared/util.h>

#include "libkmod.h"
#include "libkmod-internal.h"

#define MODULES_BUILTIN_MODINFO "modules.builtin.modinfo"

struct kmod_builtin_info {
	struct kmod_ctx *ctx;

	// The file handle.
	FILE *fp;

	// Internal buffer and its size. Used by getdelim.
	size_t bufsz;
	char *buf;
};

static int kmod_builtin_info_init(struct kmod_builtin_info *info, struct kmod_ctx *ctx)
{
	char path[PATH_MAX];
	FILE *fp = NULL;
	const char *dirname = kmod_get_dirname(ctx);
	size_t len = strlen(dirname);

	if ((len + 1 + strlen(MODULES_BUILTIN_MODINFO) + 1) >= sizeof(path))
		return -ENAMETOOLONG;
	snprintf(path, sizeof(path), "%s/" MODULES_BUILTIN_MODINFO, dirname);

	fp = fopen(path, "r");
	if (fp == NULL)
		return -errno;

	info->ctx = ctx;
	info->fp = fp;
	info->bufsz = 0;
	info->buf = NULL;

	return 0;
}

static void kmod_builtin_info_release(struct kmod_builtin_info *info)
{
	free(info->buf);
	fclose(info->fp);
}

static ssize_t get_strings(struct kmod_builtin_info *info, const char *modname,
			   struct strbuf *buf)
{
	const size_t modlen = strlen(modname);
	ssize_t count, n;

	for (count = 0; count < INTPTR_MAX;) {
		char *dot, *line;

		n = getdelim(&info->buf, &info->bufsz, '\0', info->fp);
		if (n == -1) {
			if (!feof(info->fp)) {
				count = -errno;
				ERR(info->ctx, "get_strings: %m\n");
			}
			break;
		}

		line = info->buf;
		dot = strchr(line, '.');
		if (dot == NULL) {
			count = -EINVAL;
			ERR(info->ctx, "get_strings: "
				       "unexpected string without modname prefix\n");
			return count;
		}
		if (strncmp(line, modname, modlen) || line[modlen] != '.') {
			/*
			 * If no string matched modname yet, keep searching.
			 * Otherwise this indicates that no further string will
			 * match again.
			 */
			if (count == 0)
				continue;
			break;
		}
		if (!strbuf_pushchars(buf, dot + 1) || !strbuf_pushchar(buf, '\0')) {
			count = -ENOMEM;
			ERR(info->ctx, "get_strings: "
				       "failed to append modinfo string\n");
			return count;
		}
		count++;
	}

	if (count == INTPTR_MAX) {
		count = -ENOMEM;
		ERR(info->ctx, "get_strings: "
			       "too many modinfo strings encountered\n");
		return count;
	}

	return count;
}

static char **strbuf_to_vector(struct strbuf *buf, size_t count)
{
	size_t vec_size, total_size;
	char **vector;
	char *s;
	size_t n;

	/* (string vector + NULL) * sizeof(char *) + strbuf_used() */
	if (uaddsz_overflow(count, 1, &n) ||
	    umulsz_overflow(sizeof(char *), n, &vec_size) ||
	    uaddsz_overflow(strbuf_used(buf), vec_size, &total_size))
		return NULL;

	vector = realloc(buf->bytes, total_size);
	if (vector == NULL)
		return NULL;

	memmove(vector + count + 1, vector, strbuf_used(buf));
	s = (char *)(vector + count + 1);
	strbuf_init(buf);

	for (n = 0; n < count; n++) {
		vector[n] = s;
		s += strlen(s) + 1;
	}
	vector[n] = NULL;

	return vector;
}

/* array will be allocated with strings in a single malloc, just free *array */
ssize_t kmod_builtin_get_modinfo(struct kmod_ctx *ctx, const char *modname,
				 char ***modinfo)
{
	DECLARE_STRBUF(buf);
	struct kmod_builtin_info info;
	ssize_t count;
	int ret;

	ret = kmod_builtin_info_init(&info, ctx);
	if (ret < 0)
		return ret;

	count = get_strings(&info, modname, &buf);
	if (count == 0)
		*modinfo = NULL;
	else if (count > 0) {
		*modinfo = strbuf_to_vector(&buf, (size_t)count);
		if (*modinfo == NULL) {
			count = -ENOMEM;
			ERR(ctx, "strbuf_to_vector: %s\n", strerror(ENOMEM));
		}
	}

	kmod_builtin_info_release(&info);
	return count;
}
