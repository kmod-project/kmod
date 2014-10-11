#pragma once

#include <stdbool.h>

/*
 * Buffer abstract data type
 */
struct buffer {
	char *bytes;
	unsigned size;
	unsigned used;
};

void buf_init(struct buffer *buf);
void buf_release(struct buffer *buf);
void buf_clear(struct buffer *buf);

/* Destroy buffer and return a copy as a C string */
char *buf_steal(struct buffer *buf);

/*
 * Return a C string owned by the buffer invalidated if the buffer is
 * changed).
 */
const char *buf_str(struct buffer *buf);

bool buf_pushchar(struct buffer *buf, char ch);
unsigned buf_pushchars(struct buffer *buf, const char *str);
void buf_popchar(struct buffer *buf);
void buf_popchars(struct buffer *buf, unsigned n);
