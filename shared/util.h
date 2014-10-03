#pragma once

#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <shared/macro.h>

/* string handling functions and memory allocations                         */
/* ************************************************************************ */
#define streq(a, b) (strcmp((a), (b)) == 0)
#define strstartswith(a, b) (strncmp(a, b, strlen(b)) == 0)
char *strchr_replace(char *s, int c, char r);
void *memdup(const void *p, size_t n) __attribute__((nonnull(1)));

/* read-like and fread-like functions                                       */
/* ************************************************************************ */
ssize_t read_str_safe(int fd, char *buf, size_t buflen) _must_check_ __attribute__((nonnull(2)));
ssize_t write_str_safe(int fd, const char *buf, size_t buflen) __attribute__((nonnull(2)));
int read_str_long(int fd, long *value, int base) _must_check_ __attribute__((nonnull(2)));
int read_str_ulong(int fd, unsigned long *value, int base) _must_check_ __attribute__((nonnull(2)));
char *freadline_wrapped(FILE *fp, unsigned int *linenum) __attribute__((nonnull(1)));

/* path handling functions                                                  */
/* ************************************************************************ */
bool path_is_absolute(const char *p) _must_check_ __attribute__((nonnull(1)));
char *path_make_absolute_cwd(const char *p) _must_check_ __attribute__((nonnull(1)));
int mkdir_p(const char *path, int len, mode_t mode);
int mkdir_parents(const char *path, mode_t mode);
unsigned long long stat_mstamp(const struct stat *st);
unsigned long long ts_usec(const struct timespec *ts);

/* endianess and alignments                                                 */
/* ************************************************************************ */
#define get_unaligned(ptr)			\
({						\
	struct __attribute__((packed)) {	\
		typeof(*(ptr)) __v;		\
	} *__p = (typeof(__p)) (ptr);		\
	__p->__v;				\
})

#define put_unaligned(val, ptr)			\
do {						\
	struct __attribute__((packed)) {	\
		typeof(*(ptr)) __v;		\
	} *__p = (typeof(__p)) (ptr);		\
	__p->__v = (val);			\
} while(0)

static _always_inline_ unsigned int ALIGN_POWER2(unsigned int u)
{
	return 1 << ((sizeof(u) * 8) - __builtin_clz(u - 1));
}

/* misc                                                                     */
/* ************************************************************************ */
static inline void freep(void *p) {
        free(*(void**) p);
}
#define _cleanup_free_ _cleanup_(freep)
