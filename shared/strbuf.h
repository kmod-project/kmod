#pragma once

#include <stdbool.h>
#include <stddef.h>

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
void strbuf_clear(struct strbuf *buf);

/* Destroy buffer and return a copy as a C string */
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
