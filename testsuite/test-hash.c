/*
 * Copyright (C)  2014 Intel Corporation. All rights reserved.
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

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <shared/hash.h>
#include <shared/util.h>

#include "testsuite.h"

static int freecount;

static void countfreecalls(void *v)
{
	freecount++;
}

static int test_hash_new(const struct test *t)
{
	struct hash *h = hash_new(8, NULL);
	assert_return(h != NULL, EXIT_FAILURE);
	hash_free(h);
	return 0;
}
DEFINE_TEST(test_hash_new,
		.description = "test hash_new");


static int test_hash_get_count(const struct test *t)
{
	struct hash *h = hash_new(8, NULL);
	const char *k1 = "k1", *k2 = "k2", *k3 = "k3";
	const char *v1 = "v1", *v2 = "v2", *v3 = "v3";

	hash_add(h, k1, v1);
	hash_add(h, k2, v2);
	hash_add(h, k3, v3);

	assert_return(hash_get_count(h) == 3, EXIT_FAILURE);

	hash_free(h);
	return 0;
}
DEFINE_TEST(test_hash_get_count,
		.description = "test hash_add / hash_get_count");


static int test_hash_replace(const struct test *t)
{
	struct hash *h = hash_new(8, countfreecalls);
	const char *k1 = "k1", *k2 = "k2", *k3 = "k3";
	const char *v1 = "v1", *v2 = "v2", *v3 = "v3", *v4 = "v4";
	const char *v;
	int r = 0;

	r |= hash_add(h, k1, v1);
	r |= hash_add(h, k2, v2);
	r |= hash_add(h, k3, v3);

	/* replace v1 */
	r |= hash_add(h, k1, v4);

	assert_return(r == 0, EXIT_FAILURE);
	assert_return(hash_get_count(h) == 3, EXIT_FAILURE);

	v = hash_find(h, "k1");
	assert_return(streq(v, v4), EXIT_FAILURE);

	assert_return(freecount == 1, EXIT_FAILURE);

	hash_free(h);
	return 0;
}
DEFINE_TEST(test_hash_replace,
		.description = "test hash_add replacing existing value");


static int test_hash_replace_failing(const struct test *t)
{
	struct hash *h = hash_new(8, countfreecalls);
	const char *k1 = "k1", *k2 = "k2", *k3 = "k3";
	const char *v1 = "v1", *v2 = "v2", *v3 = "v3", *v4 = "v4";
	const char *v;
	int r = 0;

	r |= hash_add(h, k1, v1);
	r |= hash_add(h, k2, v2);
	r |= hash_add(h, k3, v3);

	assert_return(r == 0, EXIT_FAILURE);

	/* replace v1 */
	r = hash_add_unique(h, k1, v4);
	assert_return(r != 0, EXIT_FAILURE);
	assert_return(hash_get_count(h) == 3, EXIT_FAILURE);

	v = hash_find(h, "k1");
	assert_return(streq(v, v1), EXIT_FAILURE);

	assert_return(freecount == 0, EXIT_FAILURE);

	hash_free(h);
	return 0;
}
DEFINE_TEST(test_hash_replace_failing,
		.description = "test hash_add_unique failing to replace existing value");


static int test_hash_iter(const struct test *t)
{
	struct hash *h = hash_new(8, NULL);
	struct hash *h2 = hash_new(8, NULL);
	const char *k1 = "k1", *k2 = "k2", *k3 = "k3";
	const char *v1 = "v1", *v2 = "v2", *v3 = "v3";
	struct hash_iter iter;
	const char *k, *v;

	hash_add(h, k1, v1);
	hash_add(h2, k1, v1);
	hash_add(h, k2, v2);
	hash_add(h2, k2, v2);
	hash_add(h, k3, v3);
	hash_add(h2, k3, v3);

	for (hash_iter_init(h, &iter);
	     hash_iter_next(&iter, &k, (const void **) &v);) {
		v2 = hash_find(h2, k);
		assert_return(v2 != NULL, EXIT_FAILURE);
		hash_del(h2, k);
	}

	assert_return(hash_get_count(h) == 3, EXIT_FAILURE);
	assert_return(hash_get_count(h2) == 0, EXIT_FAILURE);

	hash_free(h);
	hash_free(h2);
	return 0;
}
DEFINE_TEST(test_hash_iter,
		.description = "test hash_iter");


static int test_hash_iter_after_del(const struct test *t)
{
	struct hash *h = hash_new(8, NULL);
	struct hash *h2 = hash_new(8, NULL);
	const char *k1 = "k1", *k2 = "k2", *k3 = "k3";
	const char *v1 = "v1", *v2 = "v2", *v3 = "v3";
	struct hash_iter iter;
	const char *k, *v;

	hash_add(h, k1, v1);
	hash_add(h2, k1, v1);
	hash_add(h, k2, v2);
	hash_add(h2, k2, v2);
	hash_add(h, k3, v3);
	hash_add(h2, k3, v3);

	hash_del(h, k1);

	for (hash_iter_init(h, &iter);
	     hash_iter_next(&iter, &k, (const void **) &v);) {
		v2 = hash_find(h2, k);
		assert_return(v2 != NULL, EXIT_FAILURE);
		hash_del(h2, k);
	}

	assert_return(hash_get_count(h) == 2, EXIT_FAILURE);
	assert_return(hash_get_count(h2) == 1, EXIT_FAILURE);

	hash_free(h);
	hash_free(h2);
	return 0;
}
DEFINE_TEST(test_hash_iter_after_del,
		.description = "test hash_iter, after deleting element");


static int test_hash_free(const struct test *t)
{
	struct hash *h = hash_new(8, countfreecalls);
	const char *k1 = "k1", *k2 = "k2", *k3 = "k3";
	const char *v1 = "v1", *v2 = "v2", *v3 = "v3";

	hash_add(h, k1, v1);
	hash_add(h, k2, v2);
	hash_add(h, k3, v3);

	hash_del(h, k1);

	assert_return(freecount == 1, EXIT_FAILURE);

	assert_return(hash_get_count(h) == 2, EXIT_FAILURE);

	hash_free(h);

	assert_return(freecount == 3, EXIT_FAILURE);

	return 0;
}
DEFINE_TEST(test_hash_free,
		.description = "test hash_free calling free function for all values");


static int test_hash_add_unique(const struct test *t)
{
	const char *k[] = { "k1", "k2", "k3", "k4", "k5" };
	const char *v[] = { "v1", "v2", "v3", "v4", "v5" };
	unsigned int i, j, N;

	N = ARRAY_SIZE(k);
	for (i = 0; i < N; i++) {
		/* With N - 1 buckets, there'll be a bucket with more than one key. */
		struct hash *h = hash_new(N - 1, NULL);

		/* Add the keys in different orders. */
		for (j = 0; j < N; j++) {
			unsigned int idx = (j + i) % N;
			hash_add_unique(h, k[idx], v[idx]);
		}

		assert_return(hash_get_count(h) == N, EXIT_FAILURE);
		hash_free(h);
	}
	return 0;
}
DEFINE_TEST(test_hash_add_unique,
		.description = "test hash_add_unique with different key orders")


static int test_hash_massive_add_del(const struct test *t)
{
	char buf[1024 * 8];
	char *k;
	struct hash *h;
	unsigned int i, N = 1024;

	h = hash_new(8, NULL);

	k = &buf[0];
	for (i = 0; i < N; i++) {
		snprintf(k, 8, "k%d", i);
		hash_add(h, k, k);
		k += 8;
	}

	assert_return(hash_get_count(h) == N, EXIT_FAILURE);

	k = &buf[0];
	for (i = 0; i < N; i++) {
		hash_del(h, k);
		k += 8;
	}

	assert_return(hash_get_count(h) == 0, EXIT_FAILURE);

	hash_free(h);
	return 0;
}
DEFINE_TEST(test_hash_massive_add_del,
		.description = "test multiple adds followed by multiple dels")

TESTSUITE_MAIN();
