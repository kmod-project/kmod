#pragma once

#include <stdbool.h>
#include <stdlib.h>

#include <shared/macro.h>

/*
 * Buffer abstract data type
 */
struct scratchbuf {
	char *bytes;
	size_t size;
	bool need_free;
};

void scratchbuf_init(struct scratchbuf *buf, char *stackbuf, size_t size);
int scratchbuf_alloc(struct scratchbuf *buf, size_t sz);
void scratchbuf_release(struct scratchbuf *buf);

/* Return a C string */
static inline char *scratchbuf_str(struct scratchbuf *buf)
{
	return buf->bytes;
}

#define SCRATCHBUF_INITIALIZER(buf_) {			\
	.bytes = buf_,					\
	.size = sizeof(buf_) + _array_size_chk(buf_),	\
	.need_free = false,				\
}
