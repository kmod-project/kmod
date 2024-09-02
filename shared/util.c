// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2011-2013  ProFUSION embedded systems
 * Copyright (C) 2012  Lucas De Marchi <lucas.de.marchi@gmail.com>
 * Copyright (C) 2013-2014  Intel Corporation. All rights reserved.
 */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <shared/missing.h>
#include <shared/util.h>

#define USEC_PER_SEC 1000000ULL
#define NSEC_PER_USEC 1000ULL

static const struct kmod_ext {
	const char *ext;
	size_t len;
} kmod_exts[] = { { KMOD_EXTENSION_UNCOMPRESSED, sizeof(KMOD_EXTENSION_UNCOMPRESSED) - 1 },
#ifdef ENABLE_ZLIB
		  { ".ko.gz", sizeof(".ko.gz") - 1 },
#endif
#ifdef ENABLE_XZ
		  { ".ko.xz", sizeof(".ko.xz") - 1 },
#endif
#ifdef ENABLE_ZSTD
		  { ".ko.zst", sizeof(".ko.zst") - 1 },
#endif
		  {} };

/* string handling functions and memory allocations                         */
/* ************************************************************************ */

void *memdup(const void *p, size_t n)
{
	void *r = malloc(n);

	if (r == NULL)
		return NULL;

	return memcpy(r, p, n);
}

char *strchr_replace(char *s, char c, char r)
{
	char *p;

	for (p = s; *p != '\0'; p++) {
		if (*p == c)
			*p = r;
	}

	return s;
}

/* module-related functions                                                 */
/* ************************************************************************ */
int alias_normalize(const char *alias, char buf[static PATH_MAX], size_t *len)
{
	size_t i;

	for (i = 0; i < PATH_MAX - 1; i++) {
		const char c = alias[i];
		switch (c) {
		case '-':
			buf[i] = '_';
			break;
		case ']':
			return -EINVAL;
		case '[':
			while (alias[i] != ']' && alias[i] != '\0') {
				buf[i] = alias[i];
				i++;
			}

			if (alias[i] != ']')
				return -EINVAL;

			buf[i] = alias[i];
			break;
		case '\0':
			goto finish;
		default:
			buf[i] = c;
		}
	}

finish:
	buf[i] = '\0';
	if (len)
		*len = i;

	return 0;
}

/*
 * Replace dashes with underscores.
 * Dashes inside character range patterns (e.g. [0-9]) are left unchanged.
 *
 * For convenience, it returns error if @s is NULL
 */
int underscores(char *s)
{
	unsigned int i;

	if (!s)
		return -EINVAL;

	for (i = 0; s[i]; i++) {
		switch (s[i]) {
		case '-':
			s[i] = '_';
			break;
		case ']':
			return -EINVAL;
		case '[':
			i += strcspn(&s[i], "]");
			if (!s[i])
				return -EINVAL;
			break;
		}
	}

	return 0;
}

char *modname_normalize(const char *modname, char buf[static PATH_MAX], size_t *len)
{
	size_t s;

	for (s = 0; s < PATH_MAX - 1; s++) {
		const char c = modname[s];
		if (c == '-')
			buf[s] = '_';
		else if (c == '\0' || c == '.')
			break;
		else
			buf[s] = c;
	}

	buf[s] = '\0';

	if (len)
		*len = s;

	return buf;
}

char *path_to_modname(const char *path, char buf[static PATH_MAX], size_t *len)
{
	const char *modname;

	modname = basename(path);
	if (modname == NULL || modname[0] == '\0')
		return NULL;

	return modname_normalize(modname, buf, len);
}

bool path_ends_with_kmod_ext(const char *path, size_t len)
{
	const struct kmod_ext *eitr;

	for (eitr = kmod_exts; eitr->ext != NULL; eitr++) {
		if (len <= eitr->len)
			continue;
		if (streq(path + len - eitr->len, eitr->ext))
			return true;
	}

	return false;
}

/* read-like and fread-like functions                                       */
/* ************************************************************************ */
ssize_t read_str_safe(int fd, char *buf, size_t buflen)
{
	size_t todo = buflen - 1;
	size_t done = 0;

	assert_cc(EAGAIN == EWOULDBLOCK);

	do {
		ssize_t r = read(fd, buf + done, todo);

		if (r == 0)
			break;
		else if (r > 0) {
			todo -= r;
			done += r;
		} else {
			if (errno == EAGAIN || errno == EINTR)
				continue;
			else
				return -errno;
		}
	} while (todo > 0);

	buf[done] = '\0';
	return done;
}

ssize_t write_str_safe(int fd, const char *buf, size_t buflen)
{
	size_t todo = buflen;
	size_t done = 0;

	assert_cc(EAGAIN == EWOULDBLOCK);

	do {
		ssize_t r = write(fd, buf + done, todo);

		if (r == 0)
			break;
		else if (r > 0) {
			todo -= r;
			done += r;
		} else {
			if (errno == EAGAIN || errno == EINTR)
				continue;
			else
				return -errno;
		}
	} while (todo > 0);

	return done;
}

int read_str_long(int fd, long *value, int base)
{
	char buf[32], *end;
	long v;
	int err;

	*value = 0;
	err = read_str_safe(fd, buf, sizeof(buf));
	if (err < 0)
		return err;
	errno = 0;
	v = strtol(buf, &end, base);
	if (end == buf || !isspace(*end))
		return -EINVAL;

	*value = v;
	return 0;
}

int read_str_ulong(int fd, unsigned long *value, int base)
{
	char buf[32], *end;
	long v;
	int err;

	*value = 0;
	err = read_str_safe(fd, buf, sizeof(buf));
	if (err < 0)
		return err;
	errno = 0;
	v = strtoul(buf, &end, base);
	if (end == buf || !isspace(*end))
		return -EINVAL;
	*value = v;
	return 0;
}

/*
 * Read one logical line from a configuration file.
 *
 * Line endings may be escaped with backslashes, to form one logical line from
 * several physical lines.  No end of line character(s) are included in the
 * result.
 *
 * If linenum is not NULL, it is incremented by the number of physical lines
 * which have been read.
 */
char *freadline_wrapped(FILE *fp, unsigned int *linenum)
{
	size_t i = 0, size = 256;
	unsigned int n = 0;
	_cleanup_free_ char *buf = malloc(size);

	if (buf == NULL)
		return NULL;

	for (;;) {
		int ch = getc_unlocked(fp);

		switch (ch) {
		case EOF:
			if (i == 0)
				return NULL;
			/* else fall through */

		case '\n':
			n++;

			{
				char *ret = buf;
				ret[i] = '\0';
				buf = NULL;
				if (linenum)
					*linenum += n;
				return ret;
			}

		case '\\':
			ch = getc_unlocked(fp);

			if (ch == '\n') {
				n++;
				continue;
			}
			/* else fall through */

		default:
			buf[i++] = ch;

			if (i == size) {
				char *tmp;
				size *= 2;
				tmp = realloc(buf, size);
				if (!tmp)
					return NULL;
				buf = tmp;
			}
		}
	}
}

/* path handling functions                                                  */
/* ************************************************************************ */

static bool path_is_absolute(const char *p)
{
	assert(p != NULL);

	return p[0] == '/';
}

char *path_make_absolute_cwd(const char *p)
{
	_cleanup_free_ char *cwd = NULL;
	size_t plen, cwdlen;
	char *r;

	if (path_is_absolute(p))
		return strdup(p);

	cwd = get_current_dir_name();
	if (!cwd)
		return NULL;

	plen = strlen(p);
	cwdlen = strlen(cwd);

	/* cwd + '/' + p + '\0' */
	r = realloc(cwd, cwdlen + 1 + plen + 1);
	if (r == NULL)
		return NULL;

	cwd = NULL;
	r[cwdlen] = '/';
	memcpy(&r[cwdlen + 1], p, plen + 1);

	return r;
}

static inline int is_dir(const char *path)
{
	struct stat st;

	if (stat(path, &st) >= 0)
		return S_ISDIR(st.st_mode);

	return -errno;
}

int mkdir_p(const char *path, int len, mode_t mode)
{
	char *start, *end;

	start = strndupa(path, len);
	end = start + len;

	/*
	 * scan backwards, replacing '/' with '\0' while the component doesn't
	 * exist
	 */
	for (;;) {
		int r = is_dir(start);
		if (r > 0) {
			end += strlen(end);

			if (end == start + len)
				return 0;

			/* end != start, since it would be caught on the first
			 * iteration */
			*end = '/';
			break;
		} else if (r == 0)
			return -ENOTDIR;

		if (end == start)
			break;

		*end = '\0';

		/* Find the next component, backwards, discarding extra '/'*/
		while (end > start && *end != '/')
			end--;

		while (end > start && *(end - 1) == '/')
			end--;
	}

	for (; end < start + len;) {
		if (mkdir(start, mode) < 0 && errno != EEXIST)
			return -errno;

		end += strlen(end);
		*end = '/';
	}

	return 0;
}

int mkdir_parents(const char *path, mode_t mode)
{
	char *end = strrchr(path, '/');

	/* no parent directories */
	if (end == NULL)
		return 0;

	return mkdir_p(path, end - path, mode);
}

static unsigned long long ts_usec(const struct timespec *ts)
{
	return (unsigned long long)ts->tv_sec * USEC_PER_SEC +
	       (unsigned long long)ts->tv_nsec / NSEC_PER_USEC;
}

static unsigned long long ts_msec(const struct timespec *ts)
{
	return (unsigned long long)ts->tv_sec * MSEC_PER_SEC +
	       (unsigned long long)ts->tv_nsec / NSEC_PER_MSEC;
}

static struct timespec msec_ts(unsigned long long msec)
{
	struct timespec ts = {
		.tv_sec = msec / MSEC_PER_SEC,
		.tv_nsec = (msec % MSEC_PER_SEC) * NSEC_PER_MSEC,
	};

	return ts;
}

int sleep_until_msec(unsigned long long msec)
{
	struct timespec ts = msec_ts(msec);

	if (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL) < 0 &&
	    errno != EINTR)
		return -errno;

	return 0;
}

/*
 * Exponential retry backoff with tail
 */
unsigned long long get_backoff_delta_msec(unsigned long long t0, unsigned long long tend,
					  unsigned long long *delta)
{
	unsigned long long t;

	t = now_msec();

	if (!*delta)
		*delta = 1;
	else
		*delta <<= 1;

	while (t + *delta > tend)
		*delta >>= 1;

	if (!*delta && tend > t)
		*delta = tend - t;

	return t + *delta;
}

unsigned long long now_usec(void)
{
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
		return 0;

	return ts_usec(&ts);
}

unsigned long long now_msec(void)
{
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
		return 0;

	return ts_msec(&ts);
}

unsigned long long stat_mstamp(const struct stat *st)
{
#ifdef HAVE_STRUCT_STAT_ST_MTIM
	return ts_usec(&st->st_mtim);
#else
	return (unsigned long long)st->st_mtime;
#endif
}
