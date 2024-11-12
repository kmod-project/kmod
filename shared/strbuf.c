// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2011-2013  ProFUSION embedded systems
 * Copyright (C) 2014  Intel Corporation. All rights reserved.
 */

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include "util.h"
#include "strbuf.h"

#define BUF_STEP 128

static bool buf_realloc(struct strbuf *buf, size_t sz)
{
	void *tmp = realloc(buf->heap ? buf->bytes : NULL, sz);

	if (sz > 0) {
		if (tmp == NULL)
			return false;

		if (!buf->heap)
			memcpy(tmp, buf->bytes, MIN(buf->size, sz));
	}

	buf->heap = true;
	buf->bytes = tmp;
	buf->size = sz;

	return true;
}

bool strbuf_reserve_extra(struct strbuf *buf, size_t n)
{
	if (uaddsz_overflow(buf->used, n, &n))
		return false;

	if (n <= buf->size)
		return true;

	if (n % BUF_STEP)
		n = ((n / BUF_STEP) + 1) * BUF_STEP;

	return buf_realloc(buf, n);
}

void strbuf_init(struct strbuf *buf)
{
	buf->bytes = NULL;
	buf->size = 0;
	buf->used = 0;
	buf->heap = true;
}

void strbuf_release(struct strbuf *buf)
{
	if (buf->heap)
		free(buf->bytes);
}

char *strbuf_steal(struct strbuf *buf)
{
	char *bytes;

	if (!buf_realloc(buf, buf->used + 1))
		return NULL;

	bytes = buf->bytes;
	buf->bytes = NULL;
	bytes[buf->used] = '\0';

	return bytes;
}

const char *strbuf_str(struct strbuf *buf)
{
	if (!strbuf_reserve_extra(buf, 1))
		return NULL;
	buf->bytes[buf->used] = '\0';
	return buf->bytes;
}

bool strbuf_pushchar(struct strbuf *buf, char ch)
{
	if (!strbuf_reserve_extra(buf, 1))
		return false;
	buf->bytes[buf->used] = ch;
	buf->used++;
	return true;
}

size_t strbuf_pushchars(struct strbuf *buf, const char *str)
{
	size_t len;

	assert(str != NULL);
	assert(buf != NULL);

	len = strlen(str);

	if (!strbuf_reserve_extra(buf, len))
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

void strbuf_popchars(struct strbuf *buf, size_t n)
{
	assert(buf->used >= n);
	buf->used -= n;
}

void strbuf_clear(struct strbuf *buf)
{
	buf->used = 0;
}
