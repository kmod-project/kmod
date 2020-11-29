/*
 * libkmod - interface to kernel built-in modules
 *
 * Copyright (C) 2019  Alexey Gladkov <gladkov.alexey@gmail.com>
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "libkmod.h"
#include "libkmod-internal.h"

#define MODULES_BUILTIN_MODINFO "modules.builtin.modinfo"

struct kmod_builtin_iter {
	struct kmod_ctx *ctx;

	// The file descriptor.
	int file;

	// The total size in bytes.
	ssize_t size;

	// The offset of current module.
	off_t pos;

	// The offset at which the next module is located.
	off_t next;

	// Number of strings in the current block.
	ssize_t nstrings;

	// Internal buffer and its size.
	size_t bufsz;
	char *buf;
};

struct kmod_builtin_iter *kmod_builtin_iter_new(struct kmod_ctx *ctx)
{
	char path[PATH_MAX];
	int file, sv_errno;
	struct stat sb;
	struct kmod_builtin_iter *iter = NULL;
	const char *dirname = kmod_get_dirname(ctx);
	size_t len = strlen(dirname);

	file = -1;

	if ((len + 1 + strlen(MODULES_BUILTIN_MODINFO) + 1) >= PATH_MAX) {
		sv_errno = ENAMETOOLONG;
		goto fail;
	}

	snprintf(path, PATH_MAX, "%s/%s", dirname, MODULES_BUILTIN_MODINFO);

	file = open(path, O_RDONLY|O_CLOEXEC);
	if (file < 0) {
		sv_errno = errno;
		goto fail;
	}

	if (fstat(file, &sb) < 0) {
		sv_errno = errno;
		goto fail;
	}

	iter = malloc(sizeof(*iter));
	if (!iter) {
		sv_errno = ENOMEM;
		goto fail;
	}

	iter->ctx = ctx;
	iter->file = file;
	iter->size = sb.st_size;
	iter->nstrings = 0;
	iter->pos = 0;
	iter->next = 0;
	iter->bufsz = 0;
	iter->buf = NULL;

	return iter;
fail:
	if (file >= 0)
		close(file);

	errno = sv_errno;

	return iter;
}

void kmod_builtin_iter_free(struct kmod_builtin_iter *iter)
{
	close(iter->file);
	free(iter->buf);
	free(iter);
}

static off_t get_string(struct kmod_builtin_iter *iter, off_t offset,
			char **line, size_t *size)
{
	int sv_errno;
	char *nullp = NULL;
	size_t linesz = 0;

	while (!nullp) {
		char buf[BUFSIZ];
		ssize_t sz;
		size_t partsz;

		sz = pread(iter->file, buf, BUFSIZ, offset);
		if (sz < 0) {
			sv_errno = errno;
			goto fail;
		} else if (sz == 0) {
			offset = 0;
			break;
		}

		nullp = memchr(buf, '\0', (size_t) sz);
		partsz = (size_t)((nullp) ? (nullp - buf) + 1 : sz);
		offset += (off_t) partsz;

		if (iter->bufsz < linesz + partsz) {
			iter->bufsz = linesz + partsz;
			iter->buf = realloc(iter->buf, iter->bufsz);

			if (!iter->buf) {
				sv_errno = errno;
				goto fail;
			}
		}

		strncpy(iter->buf + linesz, buf, partsz);
		linesz += partsz;
	}

	if (linesz) {
		*line = iter->buf;
		*size = linesz;
	}

	return offset;
fail:
	errno = sv_errno;
	return -1;
}

bool kmod_builtin_iter_next(struct kmod_builtin_iter *iter)
{
	char *line,  *modname;
	size_t linesz;
	off_t pos, offset, modlen;

	modname = NULL;

	iter->nstrings = 0;
	offset = pos = iter->next;

	while (offset < iter->size) {
		char *dot;
		off_t len;

		offset = get_string(iter, pos, &line, &linesz);
		if (offset <= 0) {
			if (offset)
				ERR(iter->ctx, "get_string: %s\n", strerror(errno));
			pos = iter->size;
			break;
		}

		dot = strchr(line, '.');
		if (!dot) {
			ERR(iter->ctx, "kmod_builtin_iter_next: unexpected string without modname prefix\n");
			pos = iter->size;
			break;
		}

		len = dot - line;

		if (!modname) {
			modname = strdup(line);
			modlen = len;
		} else if (modlen != len || strncmp(modname, line, len)) {
			break;
		}

		iter->nstrings++;
		pos = offset;
	}

	iter->pos = iter->next;
	iter->next = pos;

	free(modname);

	return (iter->pos < iter->size);
}

bool kmod_builtin_iter_get_modname(struct kmod_builtin_iter *iter,
				char modname[static PATH_MAX])
{
	int sv_errno;
	char *line, *dot;
	size_t linesz, len;
	off_t offset;

	if (iter->pos == iter->size)
		return false;

	line = NULL;

	offset = get_string(iter, iter->pos, &line, &linesz);
	if (offset <= 0) {
		sv_errno = errno;
		if (offset)
			ERR(iter->ctx, "get_string: %s\n", strerror(errno));
		goto fail;
	}

	dot = strchr(line, '.');
	if (!dot) {
		sv_errno = errno;
		ERR(iter->ctx, "kmod_builtin_iter_get_modname: unexpected string without modname prefix\n");
		goto fail;
	}

	len = dot - line;

	if (len > PATH_MAX) {
		sv_errno = ENAMETOOLONG;
		goto fail;
	}

	strncpy(modname, line, len);
	modname[len] = '\0';

	return true;
fail:
	errno = sv_errno;
	return false;
}

/* array will be allocated with strings in a single malloc, just free *array */
ssize_t kmod_builtin_get_modinfo(struct kmod_ctx *ctx, const char *modname,
				char ***modinfo)
{
	ssize_t count = 0;
	char *s, *line = NULL;
	size_t i, n, linesz, modlen, size;
	off_t pos, offset;

	char *name = NULL;
	char buf[PATH_MAX];

	struct kmod_builtin_iter *iter = kmod_builtin_iter_new(ctx);

	if (!iter)
		return -errno;

	while (!name && kmod_builtin_iter_next(iter)) {
		if (!kmod_builtin_iter_get_modname(iter, buf)) {
			count = -errno;
			goto fail;
		}

		if (strcmp(modname, buf))
			continue;

		name = buf;
	}

	if (!name) {
		count = -ENOSYS;
		goto fail;
	}

	modlen = strlen(modname) + 1;
	count = iter->nstrings;
	size = iter->next - iter->pos - (modlen * count);

	*modinfo = malloc(size + sizeof(char *) * (count + 1));
	if (!*modinfo) {
		count = -errno;
		goto fail;
	}

	s = (char *)(*modinfo + count + 1);
	i = 0;

	n = 0;
	offset = pos = iter->pos;

	while (offset < iter->next) {
		offset = get_string(iter, pos, &line, &linesz);
		if (offset <= 0) {
			count = (offset) ? -errno : -EOF;
			free(*modinfo);
			goto fail;
		}

		strcpy(s + i, line + modlen);
		(*modinfo)[n++] = s + i;
		i += linesz - modlen;

		pos = offset;
	}
fail:
	kmod_builtin_iter_free(iter);
	return count;
}
