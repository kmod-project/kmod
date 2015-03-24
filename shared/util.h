#pragma once

#include <inttypes.h>
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
char *strchr_replace(char *s, char c, char r);
void *memdup(const void *p, size_t n) __attribute__((nonnull(1)));

/* module-related functions                                                 */
/* ************************************************************************ */
#define KMOD_EXTENSION_UNCOMPRESSED ".ko"

int alias_normalize(const char *alias, char buf[static PATH_MAX], size_t *len) _must_check_ __attribute__((nonnull(1,2)));
int underscores(char *s) _must_check_;
char *modname_normalize(const char *modname, char buf[static PATH_MAX], size_t *len) __attribute__((nonnull(1, 2)));
char *path_to_modname(const char *path, char buf[static PATH_MAX], size_t *len) __attribute__((nonnull(2)));
bool path_ends_with_kmod_ext(const char *path, size_t len) __attribute__((nonnull(1)));

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

static inline bool addu64_overflow(uint64_t a, uint64_t b, uint64_t *res)
{
#if (HAVE___BUILTIN_UADDL_OVERFLOW && HAVE___BUILTIN_UADDLL_OVERFLOW)
#if __SIZEOF_LONG__ == 8
	return __builtin_uaddl_overflow(a, b, res);
#elif __SIZEOF_LONG_LONG__ == 8
	return __builtin_uaddll_overflow(a, b, res);
#else
#error "sizeof(long long) != 8"
#endif
#endif
	*res = a + b;
	return UINT64_MAX - a < b;
}
