#pragma once

#include <stdbool.h>

/*
 * Buffer abstract data type
 */
struct strbuf {
	char *bytes;
	unsigned size;
	unsigned used;
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
unsigned strbuf_pushchars(struct strbuf *buf, const char *str);
void strbuf_popchar(struct strbuf *buf);
void strbuf_popchars(struct strbuf *buf, unsigned n);
