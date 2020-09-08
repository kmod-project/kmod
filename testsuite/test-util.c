/*
 * Copyright (C) 2012-2013  ProFUSION embedded systems
 * Copyright (C) 2012  Pedro Pedruzzi
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
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

static int alias_1(const struct test *t)
{
	static const char *input[] = {
		"test1234",
		"test[abcfoobar]2211",
		"bar[aaa][bbbb]sss",
		"kmod[p.b]lib",
		"[az]1234[AZ]",
		NULL,
	};

	char buf[PATH_MAX];
	size_t len;
	const char **alias;

	for (alias = input; *alias != NULL; alias++) {
		int ret;

		ret = alias_normalize(*alias, buf, &len);
		printf("input   %s\n", *alias);
		printf("return  %d\n", ret);

		if (ret == 0) {
			printf("len     %zu\n", len);
			printf("output  %s\n", buf);
		}

		printf("\n");
	}

	return EXIT_SUCCESS;
}
DEFINE_TEST(alias_1,
	.description = "check if alias_normalize does the right thing",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-util/",
	},
	.need_spawn = true,
	.output = {
		.out = TESTSUITE_ROOTFS "test-util/alias-correct.txt",
	});

static int test_freadline_wrapped(const struct test *t)
{
	FILE *fp = fopen("/freadline_wrapped-input.txt", "re");

	if (!fp)
		return EXIT_FAILURE;

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
	return EXIT_SUCCESS;
}
DEFINE_TEST(test_freadline_wrapped,
	.description = "check if freadline_wrapped() does the right thing",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-util/",
	},
	.need_spawn = true,
	.output = {
		.out = TESTSUITE_ROOTFS "test-util/freadline_wrapped-correct.txt",
	});

static int test_strchr_replace(const struct test *t)
{
	_cleanup_free_ char *s = strdup("this is a test string");
	const char *res = "thiC iC a teCt Ctring";

	strchr_replace(s, 's', 'C');
	assert_return(streq(s, res), EXIT_FAILURE);

	return EXIT_SUCCESS;
}
DEFINE_TEST(test_strchr_replace,
	.description = "check implementation of strchr_replace()",
	.need_spawn = false,
	);

static int test_underscores(const struct test *t)
{
	struct teststr {
		char *val;
		const char *res;
	} teststr[] = {
		{ strdup("aa-bb-cc_"), "aa_bb_cc_" },
		{ strdup("-aa-bb-cc-"), "_aa_bb_cc_" },
		{ strdup("-aa[-bb-]cc-"), "_aa[-bb-]cc_" },
		{ strdup("-aa-[bb]-cc-"), "_aa_[bb]_cc_" },
		{ strdup("-aa-[b-b]-cc-"), "_aa_[b-b]_cc_" },
		{ strdup("-aa-b[-]b-cc"), "_aa_b[-]b_cc" },
		{ }
	}, *iter;

	for (iter = &teststr[0]; iter->val != NULL; iter++) {
		_cleanup_free_ char *val = iter->val;
		underscores(val);
		assert_return(streq(val, iter->res), EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}
DEFINE_TEST(test_underscores,
	.description = "check implementation of underscores()",
	.need_spawn = false,
	);

static int test_path_ends_with_kmod_ext(const struct test *t)
{
	struct teststr {
		const char *val;
		bool res;
	} teststr[] = {
		{ "/bla.ko", true },
#ifdef ENABLE_ZLIB
		{ "/bla.ko.gz", true },
#endif
#ifdef ENABLE_XZ
		{ "/bla.ko.xz", true },
#endif
#ifdef ENABLE_ZSTD
		{ "/bla.ko.zst", true },
#endif
		{ "/bla.ko.x", false },
		{ "/bla.ko.", false },
		{ "/bla.koz", false },
		{ "/b", false },
		{ }
	}, *iter;

	for (iter = &teststr[0]; iter->val != NULL; iter++) {
		assert_return(path_ends_with_kmod_ext(iter->val,
						      strlen(iter->val)) == iter->res,
			      EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}
DEFINE_TEST(test_path_ends_with_kmod_ext,
	.description = "check implementation of path_ends_with_kmod_ext()",
	.need_spawn = false,
	);

#define TEST_WRITE_STR_SAFE_FILE "/write-str-safe"
#define TEST_WRITE_STR_SAFE_PATH TESTSUITE_ROOTFS "test-util2/" TEST_WRITE_STR_SAFE_FILE
static int test_write_str_safe(const struct test *t)
{
	const char *s = "test";
	int fd;

	fd = open(TEST_WRITE_STR_SAFE_FILE ".txt", O_CREAT|O_TRUNC|O_WRONLY, 0644);
	assert_return(fd >= 0, EXIT_FAILURE);

	write_str_safe(fd, s, strlen(s));
	close(fd);

	return EXIT_SUCCESS;
}
DEFINE_TEST(test_write_str_safe,
	.description = "check implementation of write_str_safe()",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-util2/",
	},
	.need_spawn = true,
	.output = {
		.files = (const struct keyval[]) {
			{ TEST_WRITE_STR_SAFE_PATH ".txt",
			  TEST_WRITE_STR_SAFE_PATH "-correct.txt" },
			{ }
		},
	});

static int test_addu64_overflow(const struct test *t)
{
	uint64_t res;
	bool overflow;

	overflow = addu64_overflow(UINT64_MAX - 1, 1, &res);
	assert_return(!overflow, EXIT_FAILURE);
	assert_return(res == UINT64_MAX, EXIT_FAILURE);

	overflow = addu64_overflow(UINT64_MAX, 1, &res);
	assert_return(overflow, EXIT_FAILURE);

	return EXIT_SUCCESS;
}
DEFINE_TEST(test_addu64_overflow,
	.description = "check implementation of addu4_overflow()",
	.need_spawn = false,
	);


TESTSUITE_MAIN();
