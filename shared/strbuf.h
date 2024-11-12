#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "macro.h"

/*
 * Buffer abstract data type
 */
struct strbuf {
	char *bytes;
	size_t size;
	size_t used;
};

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
 * Return a C string owned by the buffer invalidated if the buffer is
 * changed).
 */
const char *strbuf_str(struct strbuf *buf);

bool strbuf_pushchar(struct strbuf *buf, char ch);
size_t strbuf_pushchars(struct strbuf *buf, const char *str);

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
