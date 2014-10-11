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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "util.h"
#include "strbuf.h"

#define BUF_STEP (2048)

static bool buf_grow(struct buffer *buf, size_t newsize)
{
	void *tmp;
	size_t sz;

	if (newsize % BUF_STEP == 0)
		sz = newsize;
	else
		sz = ((newsize / BUF_STEP) + 1) * BUF_STEP;

	if (buf->size == sz)
		return true;

	tmp = realloc(buf->bytes, sz);
	if (sz > 0 && tmp == NULL)
		return false;
	buf->bytes = tmp;
	buf->size = sz;
	return true;
}

void buf_init(struct buffer *buf)
{
	buf->bytes = NULL;
	buf->size = 0;
	buf->used = 0;
}

void buf_release(struct buffer *buf)
{
	free(buf->bytes);
}

char *buf_steal(struct buffer *buf)
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

const char *buf_str(struct buffer *buf)
{
	if (!buf_grow(buf, buf->used + 1))
		return NULL;
	buf->bytes[buf->used] = '\0';
	return buf->bytes;
}

bool buf_pushchar(struct buffer *buf, char ch)
{
	if (!buf_grow(buf, buf->used + 1))
		return false;
	buf->bytes[buf->used] = ch;
	buf->used++;
	return true;
}

unsigned buf_pushchars(struct buffer *buf, const char *str)
{
	unsigned i = 0;
	int ch;

	while ((ch = str[i])) {
		buf_pushchar(buf, ch);
		i++;
	}

	return i;
}

void buf_popchar(struct buffer *buf)
{
	assert(buf->used > 0);
	buf->used--;
}

void buf_popchars(struct buffer *buf, unsigned n)
{
	assert(buf->used >= n);
	buf->used -= n;
}

void buf_clear(struct buffer *buf)
{
	buf->used = 0;
}

