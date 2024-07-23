// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2016  Intel Corporation. All rights reserved.
 */
#include "scratchbuf.h"

#include <errno.h>
#include <string.h>

void scratchbuf_init(struct scratchbuf *buf, char *stackbuf, size_t size)
{
	buf->bytes = stackbuf;
	buf->size = size;
	buf->need_free = false;
}

int scratchbuf_alloc(struct scratchbuf *buf, size_t size)
{
	char *tmp;

	if (size <= buf->size)
		return 0;

	if (buf->need_free) {
		tmp = realloc(buf->bytes, size);
		if (tmp == NULL)
			return -ENOMEM;
	} else {
		tmp = malloc(size);
		if (tmp == NULL)
			return -ENOMEM;
		memcpy(tmp, buf->bytes, buf->size);
	}

	buf->size = size;
	buf->bytes = tmp;
	buf->need_free = true;

	return 0;
}

void scratchbuf_release(struct scratchbuf *buf)
{
	if (buf->need_free)
		free(buf->bytes);
}
