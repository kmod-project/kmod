/*
 * kmod - interface to kernel module operations
 *
 * Copyright (C) 2016  Intel Corporation. All rights reserved.
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
