// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2012-2013  ProFUSION embedded systems
 * Copyright (C) 2012  Pedro Pedruzzi
 */

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <shared/util.h>

#include "testsuite.h"

static int alias_1(void)
{
	static const char *const aliases[] = {
		// clang-format off
		"test1234",
		"test[abcfoobar]2211",
		"bar[aaa][bbbb]sss",
		"kmod[p.b]lib",
		"[az]1234[AZ]",
		// clang-format on
	};

	char buf[PATH_MAX];
	size_t len;

	for (size_t i = 0; i < ARRAY_SIZE(aliases); i++) {
		int ret;

		ret = alias_normalize(aliases[i], buf, &len);
		printf("input   %s\n", aliases[i]);
		printf("return  %d\n", ret);

		if (ret == 0) {
			printf("len     %zu\n", len);
			printf("output  %s\n", buf);
		}

		printf("\n");
	}

	return 0;
}
DEFINE_TEST(alias_1,
	.description = "check if alias_normalize does the right thing",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-util/",
	},
	.output = {
		.out = TESTSUITE_ROOTFS "test-util/alias-correct.txt",
	});

static int test_freadline_wrapped(void)
{
	FILE *fp = fopen("/freadline_wrapped-input.txt", "re");

	TS_ASSERT(fp != NULL);

	while (!feof(fp) && !ferror(fp)) {
		unsigned int num = 0;
		char *s = freadline_wrapped(fp, &num);
		if (!s)
			break;
		puts(s);
		free(s);
		printf("%u\n", num);
	}

	fclose(fp);
	return 0;
}
DEFINE_TEST(test_freadline_wrapped,
	.description = "check if freadline_wrapped() does the right thing",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-util/",
	},
	.output = {
		.out = TESTSUITE_ROOTFS "test-util/freadline_wrapped-correct.txt",
	});

static int test_strchr_replace(void)
{
	_cleanup_free_ char *s = strdup("this is a test string");
	const char *res = "thiC iC a teCt Ctring";

	strchr_replace(s, 's', 'C');
	TS_ASSERT(streq(s, res));

	return 0;
}
DEFINE_TEST(test_strchr_replace,
	    .description = "check implementation of strchr_replace()");

static int test_underscores(void)
{
	static const struct teststr {
		const char *val;
		const char *res;
	} teststr[] = {
		// clang-format off
		{ "aa-bb-cc_", "aa_bb_cc_" },
		{ "-aa-bb-cc-", "_aa_bb_cc_" },
		{ "-aa[-bb-]cc-", "_aa[-bb-]cc_" },
		{ "-aa-[bb]-cc-", "_aa_[bb]_cc_" },
		{ "-aa-[b-b]-cc-", "_aa_[b-b]_cc_" },
		{ "-aa-b[-]b-cc", "_aa_b[-]b_cc" },
		// clang-format on
	};

	for (size_t i = 0; i < ARRAY_SIZE(teststr); i++) {
		_cleanup_free_ char *val = strdup(teststr[i].val);
		TS_ASSERT(val != NULL);
		TS_ASSERT(!underscores(val));
		TS_ASSERT(streq(val, teststr[i].res));
	}

	return 0;
}
DEFINE_TEST(test_underscores, .description = "check implementation of underscores()");

static int test_path_ends_with_kmod_ext(void)
{
	static const struct teststr {
		const char *val;
		bool res;
	} teststr[] = {
		// clang-format off
		{ "/bla.ko", true },
#if ENABLE_ZLIB
		{ "/bla.ko.gz", true },
#endif
#if ENABLE_XZ
		{ "/bla.ko.xz", true },
#endif
#if ENABLE_ZSTD
		{ "/bla.ko.zst", true },
#endif
		{ "/bla.ko.x", false },
		{ "/bla.ko.", false },
		{ "/bla.koz", false },
		{ "/b", false },
		// clang-format on
	};

	for (size_t i = 0; i < ARRAY_SIZE(teststr); i++) {
		TS_ASSERT(
			path_ends_with_kmod_ext(teststr[i].val, strlen(teststr[i].val)) ==
			teststr[i].res);
	}

	return 0;
}
DEFINE_TEST(test_path_ends_with_kmod_ext,
	    .description = "check implementation of path_ends_with_kmod_ext()");

#define TEST_WRITE_STR_SAFE_FILE "/write-str-safe"
#define TEST_WRITE_STR_SAFE_PATH TESTSUITE_ROOTFS "test-util2/" TEST_WRITE_STR_SAFE_FILE
static int test_write_str_safe(void)
{
	const char *s = "test";
	int fd;

	fd = open(TEST_WRITE_STR_SAFE_FILE ".txt", O_CREAT | O_TRUNC | O_WRONLY, 0644);
	TS_ASSERT(fd >= 0);

	write_str_safe(fd, s, strlen(s));
	close(fd);

	return 0;
}
DEFINE_TEST(test_write_str_safe,
	.description = "check implementation of write_str_safe()",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-util2/",
	},
	.output = {
		.files = (const struct keyval[]) {
			{ TEST_WRITE_STR_SAFE_PATH ".txt",
			  TEST_WRITE_STR_SAFE_PATH "-correct.txt" },
			{ },
		},
	});

static int test_uadd32_overflow(void)
{
	uint32_t res;
	bool overflow;

	overflow = uadd32_overflow(UINT32_MAX - 1, 1, &res);
	TS_ASSERT(!overflow);
	TS_ASSERT(res == UINT32_MAX);

	overflow = uadd32_overflow(UINT32_MAX, 1, &res);
	TS_ASSERT(overflow);

	return 0;
}
DEFINE_TEST(test_uadd32_overflow,
	    .description = "check implementation of uadd32_overflow()");

static int test_uadd64_overflow(void)
{
	uint64_t res;
	bool overflow;

	overflow = uadd64_overflow(UINT64_MAX - 1, 1, &res);
	TS_ASSERT(!overflow);
	TS_ASSERT(res == UINT64_MAX);

	overflow = uadd64_overflow(UINT64_MAX, 1, &res);
	TS_ASSERT(overflow);

	return 0;
}
DEFINE_TEST(test_uadd64_overflow,
	    .description = "check implementation of uadd64_overflow()");

static int test_umul32_overflow(void)
{
	uint32_t res;
	bool overflow;

	overflow = umul32_overflow(UINT32_MAX / 0x10, 0x10, &res);
	TS_ASSERT(!overflow);
	TS_ASSERT(res == (UINT32_MAX & ~0xf));

	overflow = umul32_overflow(UINT32_MAX, 0x10, &res);
	TS_ASSERT(overflow);

	return 0;
}
DEFINE_TEST(test_umul32_overflow,
	    .description = "check implementation of umul32_overflow()");

static int test_umul64_overflow(void)
{
	uint64_t res;
	bool overflow;

	overflow = umul64_overflow(UINT64_MAX / 0x10, 0x10, &res);
	TS_ASSERT(!overflow);
	TS_ASSERT(res == (UINT64_MAX & ~0xf));

	overflow = umul64_overflow(UINT64_MAX, 0x10, &res);
	TS_ASSERT(overflow);

	return 0;
}
DEFINE_TEST(test_umul64_overflow,
	    .description = "check implementation of umul64_overflow()");

static int test_backoff_time(void)
{
	unsigned long long delta = 0;

	/* Check exponential increments */
	get_backoff_delta_msec(now_msec() + 10, &delta);
	TS_ASSERT(delta == 1);
	get_backoff_delta_msec(now_msec() + 10, &delta);
	TS_ASSERT(delta == 2);
	get_backoff_delta_msec(now_msec() + 10, &delta);
	TS_ASSERT(delta == 4);
	get_backoff_delta_msec(now_msec() + 10, &delta);
	TS_ASSERT(delta == 8);
	get_backoff_delta_msec(now_msec() + 10, &delta);
	TS_ASSERT(delta == 8);

	{
		/* Check tail */
		delta = 4;
		get_backoff_delta_msec(now_msec() + 3, &delta);
		TS_ASSERT(delta == 2);
		get_backoff_delta_msec(now_msec() + 1, &delta);
		TS_ASSERT(delta == 1);
		get_backoff_delta_msec(now_msec(), &delta);
		TS_ASSERT(delta == 0);
	}

	/* Check when end time has passed */
	delta = 0;
	get_backoff_delta_msec(now_msec() - 10, &delta);
	TS_ASSERT(delta == 0);

	return 0;
}
DEFINE_TEST(test_backoff_time,
	    .description = "check implementation of get_backoff_delta_msec()");

TESTSUITE_MAIN();
