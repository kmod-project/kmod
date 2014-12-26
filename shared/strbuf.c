/*
 * libkmod - interface to kernel module operations
 *
 * Copyright (C) 2011-2013  ProFUSION embedded systems
 * Copyright (C) 2014  Intel Corporation. All rights reserved.
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

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "strbuf.h"

#define BUF_STEP 128

static bool buf_grow(struct strbuf *buf, size_t newsize)
{
	void *tmp;
	size_t sz;

	if (newsize <= buf->size)
		return true;

	if (newsize % BUF_STEP == 0)
		sz = newsize;
	else
		sz = ((newsize / BUF_STEP) + 1) * BUF_STEP;

	tmp = realloc(buf->bytes, sz);
	if (sz > 0 && tmp == NULL)
		return false;
	buf->bytes = tmp;
	buf->size = sz;
	return true;
}

void strbuf_init(struct strbuf *buf)
{
	buf->bytes = NULL;
	buf->size = 0;
	buf->used = 0;
}

void strbuf_release(struct strbuf *buf)
{
	free(buf->bytes);
}

char *strbuf_steal(struct strbuf *buf)
{
	char *bytes;

	bytes = realloc(buf->bytes, buf->used + 1);
	if (!bytes) {
		free(buf->bytes);
		return NULL;
	}
	bytes[buf->used] = '\0';
	return bytes;
}

const char *strbuf_str(struct strbuf *buf)
{
	if (!buf_grow(buf, buf->used + 1))
		return NULL;
	buf->bytes[buf->used] = '\0';
	return buf->bytes;
}

bool strbuf_pushchar(struct strbuf *buf, char ch)
{
	if (!buf_grow(buf, buf->used + 1))
		return false;
	buf->bytes[buf->used] = ch;
	buf->used++;
	return true;
}

unsigned strbuf_pushchars(struct strbuf *buf, const char *str)
{
	unsigned int len;

	assert(str != NULL);
	assert(buf != NULL);

	len = strlen(str);

	if (!buf_grow(buf, buf->used + len))
		return 0;

	memcpy(buf->bytes + buf->used, str, len);
	buf->used += len;

	return len;
}

void strbuf_popchar(struct strbuf *buf)
{
	assert(buf->used > 0);
	buf->used--;
}

void strbuf_popchars(struct strbuf *buf, unsigned n)
{
	assert(buf->used >= n);
	buf->used -= n;
}

void strbuf_clear(struct strbuf *buf)
{
	buf->used = 0;
}

