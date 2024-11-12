#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <alloca.h>

#include "macro.h"

/*
 * Buffer abstract data type
 */
struct strbuf {
	char *bytes;
	size_t size;
	size_t used;
	bool heap;
};

/*
 * Declare and initialize strbuf without any initial storage
 */
#define DECLARE_STRBUF(name__)                    \
	_cleanup_strbuf_ struct strbuf name__ = { \
		.heap = true,                     \
	}

/*
 * Declare and initialize strbuf with an initial buffer on stack. The @sz__ must be a
 * build-time constant, as if the buffer had been declared on stack.
 */
#define DECLARE_STRBUF_WITH_STACK(name__, sz__)   \
	assert_cc(__builtin_constant_p(sz__));    \
	char name__##_storage__[sz__];            \
	_cleanup_strbuf_ struct strbuf name__ = { \
		.bytes = name__##_storage__,      \
		.size = sz__,                     \
	}

void strbuf_init(struct strbuf *buf);

void strbuf_release(struct strbuf *buf);
#define _cleanup_strbuf_ _cleanup_(strbuf_release)

void strbuf_clear(struct strbuf *buf);

/*
 * Return a copy as a C string, guaranteed to be nul-terminated. On success, the @buf
 * becomes invalid and shouldn't be used anymore, except for an (optional) call to
 * strbuf_release() which still does the right thing on an invalidated buffer. On failure,
 * NULL is returned and the buffer remains valid: strbuf_release() should be called.
 * Consider using _cleanup_strbuf_ attribute to release the buffer as needed.
 *
 * The copy may use the same underlying storage as the buffer and should be free'd later
 * with free().
 */
char *strbuf_steal(struct strbuf *buf);

/*
 * Return a C string owned by the buffer. It becomes an invalid
 * pointer if strbuf is changed. It may also not survive a return
 * from current function if it was initialized with stack space
 */
const char *strbuf_str(struct strbuf *buf);

/*
 * Reserve enough space for @n bytes, ensuring additional pushes up to @n bytes
 * don't cause re-allocations
 */
bool strbuf_reserve_extra(struct strbuf *buf, size_t n);

bool strbuf_pushchar(struct strbuf *buf, char ch);
size_t strbuf_pushmem(struct strbuf *buf, const char *src, size_t sz);
static inline size_t strbuf_pushchars(struct strbuf *buf, const char *str)
{
	return strbuf_pushmem(buf, str, strlen(str));
}

/*
 * Remove the last char from buf.
 *
 * No reallocation is done, so it's guaranteed @buf will have at least 1 char available to
 * be filled after this call, as long as @buf wasn't empty.
 */
void strbuf_popchar(struct strbuf *buf);

/*
 * Remove the last @n chars from buf.
 *
 * No reallocation is done, so it's guaranteed @buf will have at least @n chars available
 * to be filled after this call, as long as @buf had @n chars allocated before.
 *
 * Example:
 *
 * 	struct strbuf buf;
 * 	strbuf_init(&buf);
 * 	strbuf_pushchars(&buf, "foobar");
 * 	strbuf_popchars(&buf, 5);
 *
 * After these calls @buf is [ 'f', x, x, x, ... ], where "x" means undefined. However it's
 * guaranteed to have (at least) 5 chars available without needing to reallocate.
 */
void strbuf_popchars(struct strbuf *buf, size_t n);
