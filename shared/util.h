#pragma once

#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#include <shared/macro.h>

/* string handling functions and memory allocations                         */
/* ************************************************************************ */
#define streq(a, b) (strcmp((a), (b)) == 0)
#define strstartswith(a, b) (strncmp(a, b, strlen(b)) == 0)
char *strchr_replace(char *s, char c, char r);
_nonnull_all_ void *memdup(const void *p, size_t n);

/* module-related functions                                                 */
/* ************************************************************************ */
#define KMOD_EXTENSION_UNCOMPRESSED ".ko"

_must_check_ _nonnull_(1, 2) int alias_normalize(const char *alias,
						 char buf[static PATH_MAX], size_t *len);
_must_check_ int underscores(char *s);
_nonnull_(1, 2) char *modname_normalize(const char *modname, char buf[static PATH_MAX],
					size_t *len);
_nonnull_(2) char *path_to_modname(const char *path, char buf[static PATH_MAX],
				   size_t *len);
_nonnull_all_ bool path_ends_with_kmod_ext(const char *path, size_t len);

/* read-like and fread-like functions                                       */
/* ************************************************************************ */
_must_check_ _nonnull_(2) ssize_t pread_str_safe(int fd, char *buf, size_t buflen,
						 off_t off);
_must_check_ _nonnull_(2) ssize_t read_str_safe(int fd, char *buf, size_t buflen);
_nonnull_(2) ssize_t write_str_safe(int fd, const char *buf, size_t buflen);
_must_check_ _nonnull_(2) int read_str_long(int fd, long *value, int base);
_must_check_ _nonnull_(2) int read_str_ulong(int fd, unsigned long *value, int base);
_nonnull_(1) char *freadline_wrapped(FILE *fp, unsigned int *linenum);

/* path handling functions                                                  */
/* ************************************************************************ */
_must_check_ _nonnull_all_ char *path_make_absolute_cwd(const char *p);
static inline _must_check_ _nonnull_all_ bool path_is_absolute(const char *p)
{
	return p[0] == '/';
}

int mkdir_p(const char *path, int len, mode_t mode);
int mkdir_parents(const char *path, mode_t mode);
unsigned long long stat_mstamp(const struct stat *st);
_nonnull_all_ int fd_lookup_path(int fd, char *path, size_t pathlen);

/* time-related functions
 * ************************************************************************ */
#define USEC_PER_SEC 1000000ULL
#define USEC_PER_MSEC 1000ULL
#define MSEC_PER_SEC 1000ULL
#define NSEC_PER_MSEC 1000000ULL

unsigned long long ts_usec(const struct timespec *ts);
unsigned long long now_usec(void);
unsigned long long now_msec(void);
int sleep_until_msec(unsigned long long msec);
unsigned long long get_backoff_delta_msec(unsigned long long tend,
					  unsigned long long *delta);

/* endianness and alignments                                                */
/* ************************************************************************ */
#define get_unaligned(ptr)                       \
	({                                       \
		struct __attribute__((packed)) { \
			typeof(*(ptr)) __v;      \
		} *__p = (typeof(__p))(ptr);     \
		__p->__v;                        \
	})

#define put_unaligned(val, ptr)                  \
	do {                                     \
		struct __attribute__((packed)) { \
			typeof(*(ptr)) __v;      \
		} *__p = (typeof(__p))(ptr);     \
		__p->__v = (val);                \
	} while (0)

static inline unsigned int align_power2(unsigned int u)
{
	return 1 << ((sizeof(u) * 8) - __builtin_clz(u - 1));
}

/* misc                                                                     */
/* ************************************************************************ */

static inline bool uadd32_overflow(uint32_t a, uint32_t b, uint32_t *res)
{
#if (HAVE___BUILTIN_UADD_OVERFLOW && __SIZEOF_INT__ == 4)
	return __builtin_uadd_overflow(a, b, res);
#else
	*res = a + b;
	return UINT32_MAX - a < b;
#endif
}

static inline bool uadd64_overflow(uint64_t a, uint64_t b, uint64_t *res)
{
#if (HAVE___BUILTIN_UADDL_OVERFLOW && __SIZEOF_LONG__ == 8)
	return __builtin_uaddl_overflow(a, b, res);
#elif (HAVE___BUILTIN_UADDLL_OVERFLOW && __SIZEOF_LONG_LONG__ == 8)
	return __builtin_uaddll_overflow(a, b, res);
#else
	*res = a + b;
	return UINT64_MAX - a < b;
#endif
}

static inline bool uaddsz_overflow(size_t a, size_t b, size_t *res)
{
#if __SIZEOF_SIZE_T__ == 8
	return uadd64_overflow(a, b, res);
#elif __SIZEOF_SIZE_T__ == 4
	return uadd32_overflow(a, b, res);
#else
#error "Unknown sizeof(size_t)"
#endif
}

static inline bool umul32_overflow(uint32_t a, uint32_t b, uint32_t *res)
{
#if (HAVE___BUILTIN_UMUL_OVERFLOW && __SIZEOF_INT__ == 4)
	return __builtin_umul_overflow(a, b, res);
#else
	*res = a * b;
	return UINT32_MAX / a < b;
#endif
}

static inline bool umul64_overflow(uint64_t a, uint64_t b, uint64_t *res)
{
#if (HAVE___BUILTIN_UMULL_OVERFLOW && __SIZEOF_LONG__ == 8)
	return __builtin_umull_overflow(a, b, res);
#elif (HAVE___BUILTIN_UMULLL_OVERFLOW && __SIZEOF_LONG_LONG__ == 8)
	return __builtin_umulll_overflow(a, b, res);
#else
	*res = a * b;
	return UINT64_MAX / a < b;
#endif
}

static inline bool umulll_overflow(unsigned long long a, unsigned long long b,
				   unsigned long long *res)
{
#if (HAVE___BUILTIN_UMULLL_OVERFLOW)
	return __builtin_umulll_overflow(a, b, res);
#else
	*res = a * b;
	return UINT64_MAX / a < b;
#endif
}

static inline bool umulsz_overflow(size_t a, size_t b, size_t *res)
{
#if __SIZEOF_SIZE_T__ == 8
	return umul64_overflow(a, b, res);
#elif __SIZEOF_SIZE_T__ == 4
	return umul32_overflow(a, b, res);
#else
#error "Unknown sizeof(size_t)"
#endif
}

#define TAKE_PTR(x)                \
	({                         \
		typeof(x) x__ = x; \
		(x) = NULL;        \
		x__;               \
	})

/* dlfcn helpers                                                            */
/* ************************************************************************ */

/*
 * Load many various symbols from @filename.
 * @dlp: pointer to the previous results of this call: it's set when it succeeds
 * @filename: the library to dlopen() and look for symbols
 * @...: or 1 more tuples created by DLSYM_ARG() with ( &var, "symbol name" ).
 */
_sentinel_ int dlsym_many(void **dlp, const char *filename, ...);

/*
 * Helper to create tuples passed as arguments to dlsym_many().
 * @symbol__: symbol to create arguments for. Example: DLSYM_ARG(foo) expands to
 * `&sym_foo, "foo"`
 */
#define DLSYM_ARG(symbol__) &sym_##symbol__, STRINGIFY(symbol__),

/* For symbols being dynamically loaded */
#define DECLARE_DLSYM(symbol) static typeof(symbol) *sym_##symbol

/* Pointer indirection to support linking directly */
#define DECLARE_PTRSYM(symbol) static typeof(symbol) *sym_##symbol = symbol

/*
 * Helper defines, to be done locally before including this header to switch between
 * implementations
 */
#if defined(DLSYM_LOCALLY_ENABLED) && DLSYM_LOCALLY_ENABLED
#define DECLARE_SYM(sym__) DECLARE_DLSYM(sym__);
#else
#define DECLARE_SYM(sym__) DECLARE_PTRSYM(sym__);
#endif
