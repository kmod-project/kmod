// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C)  2014 Intel Corporation. All rights reserved.
 */

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <shared/array.h>
#include <shared/hash.h>
#include <shared/util.h>

#include "testsuite.h"

static int test_array_append1(void)
{
	struct array array;
	const char *c1 = "test1";

	array_init(&array, 2);
	array_append(&array, c1);
	TS_ASSERT(array.count == 1);
	TS_ASSERT(array.array[0] == c1);
	array_free_array(&array);

	return 0;
}
DEFINE_TEST(test_array_append1, .description = "test simple array append");

static int test_array_append2(void)
{
	struct array array;
	const char *c1 = "test1";
	const char *c2 = "test2";
	const char *c3 = "test3";

	array_init(&array, 2);
	array_append(&array, c1);
	array_append(&array, c2);
	array_append(&array, c3);
	TS_ASSERT(array.count == 3);
	TS_ASSERT(array.array[0] == c1);
	TS_ASSERT(array.array[1] == c2);
	TS_ASSERT(array.array[2] == c3);
	array_free_array(&array);

	return 0;
}
DEFINE_TEST(test_array_append2, .description = "test array append over step");

static int test_array_append_unique(void)
{
	struct array array;
	const char *c1 = "test1";
	const char *c2 = "test2";
	const char *c3 = "test3";

	array_init(&array, 2);
	array_append_unique(&array, c1);
	array_append_unique(&array, c2);
	array_append_unique(&array, c3);
	array_append_unique(&array, c3);
	array_append_unique(&array, c2);
	array_append_unique(&array, c1);
	TS_ASSERT(array.count == 3);
	TS_ASSERT(array.array[0] == c1);
	TS_ASSERT(array.array[1] == c2);
	TS_ASSERT(array.array[2] == c3);
	array_free_array(&array);

	return 0;
}
DEFINE_TEST(test_array_append_unique, .description = "test array append unique");

static int strptrcmp(const void *pa, const void *pb)
{
	const char *a = *(const char **)pa;
	const char *b = *(const char **)pb;

	return strcmp(a, b);
}

static int test_array_sort(void)
{
	struct array array;
	const char *c1 = "test1";
	const char *c2 = "test2";
	const char *c3 = "test3";

	array_init(&array, 2);
	array_append(&array, c1);
	array_append(&array, c2);
	array_append(&array, c3);
	array_append(&array, c2);
	array_append(&array, c3);
	array_append(&array, c1);
	array_sort(&array, strptrcmp);
	TS_ASSERT(array.count == 6);
	TS_ASSERT(array.array[0] == c1);
	TS_ASSERT(array.array[1] == c1);
	TS_ASSERT(array.array[2] == c2);
	TS_ASSERT(array.array[3] == c2);
	TS_ASSERT(array.array[4] == c3);
	TS_ASSERT(array.array[5] == c3);
	array_free_array(&array);

	return 0;
}
DEFINE_TEST(test_array_sort, .description = "test array sort");

static int test_array_remove_at(void)
{
	struct array array;
	const char *c1 = "test1";
	const char *c2 = "test2";
	const char *c3 = "test3";

	array_init(&array, 2);
	array_append(&array, c1);
	array_append(&array, c2);
	array_append(&array, c3);

	array_remove_at(&array, 2);
	TS_ASSERT(array.count == 2);
	TS_ASSERT(array.array[0] == c1);
	TS_ASSERT(array.array[1] == c2);
	TS_ASSERT(array.total == 4);

	array_remove_at(&array, 0);
	TS_ASSERT(array.count == 1);
	TS_ASSERT(array.array[0] == c2);
	TS_ASSERT(array.total == 2);

	array_remove_at(&array, 0);
	TS_ASSERT(array.count == 0);
	TS_ASSERT(array.total == 2);

	array_append(&array, c1);
	array_append(&array, c2);
	array_append(&array, c3);

	array_remove_at(&array, 1);
	TS_ASSERT(array.count == 2);
	TS_ASSERT(array.array[0] == c1);
	TS_ASSERT(array.array[1] == c3);
	TS_ASSERT(array.total == 4);

	array_free_array(&array);

	return 0;
}
DEFINE_TEST(test_array_remove_at, .description = "test array remove at");

static int test_array_pop(void)
{
	struct array array;
	const char *c1 = "test1";
	const char *c2 = "test2";
	const char *c3 = "test3";

	array_init(&array, 2);
	array_append(&array, c1);
	array_append(&array, c2);
	array_append(&array, c3);

	array_pop(&array);

	TS_ASSERT(array.count == 2);
	TS_ASSERT(array.array[0] == c1);
	TS_ASSERT(array.array[1] == c2);

	array_pop(&array);
	array_pop(&array);

	TS_ASSERT(array.count == 0);

	array_pop(&array);

	TS_ASSERT(array.count == 0);

	array_free_array(&array);

	return 0;
}

DEFINE_TEST(test_array_pop, .description = "test array pop");

/* hash sub-group */

static int freecount;

static void countfreecalls(_maybe_unused_ void *v)
{
	freecount++;
}

static int test_hash_new(void)
{
	struct hash *h = hash_new(8, NULL);
	TS_ASSERT(h != NULL);
	hash_free(h);
	return 0;
}
DEFINE_TEST(test_hash_new, .description = "test hash_new");

static int test_hash_get_count(void)
{
	struct hash *h = hash_new(8, NULL);
	const char *k1 = "k1", *k2 = "k2", *k3 = "k3";
	const char *v1 = "v1", *v2 = "v2", *v3 = "v3";

	hash_add(h, k1, v1);
	hash_add(h, k2, v2);
	hash_add(h, k3, v3);

	TS_ASSERT(hash_get_count(h) == 3);

	hash_free(h);
	return 0;
}
DEFINE_TEST(test_hash_get_count, .description = "test hash_add / hash_get_count");

static int test_hash_replace(void)
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

	TS_ASSERT(r == 0);
	TS_ASSERT(hash_get_count(h) == 3);

	v = hash_find(h, "k1");
	TS_ASSERT(streq(v, v4));

	TS_ASSERT(freecount == 1);

	hash_free(h);
	return 0;
}
DEFINE_TEST(test_hash_replace, .description = "test hash_add replacing existing value");

static int test_hash_replace_failing(void)
{
	struct hash *h = hash_new(8, countfreecalls);
	const char *k1 = "k1", *k2 = "k2", *k3 = "k3";
	const char *v1 = "v1", *v2 = "v2", *v3 = "v3", *v4 = "v4";
	const char *v;
	int r = 0;

	r |= hash_add(h, k1, v1);
	r |= hash_add(h, k2, v2);
	r |= hash_add(h, k3, v3);

	TS_ASSERT(r == 0);

	/* replace v1 */
	r = hash_add_unique(h, k1, v4);
	TS_ASSERT(r != 0);
	TS_ASSERT(hash_get_count(h) == 3);

	v = hash_find(h, "k1");
	TS_ASSERT(streq(v, v1));

	TS_ASSERT(freecount == 0);

	hash_free(h);
	return 0;
}
DEFINE_TEST(test_hash_replace_failing,
	    .description = "test hash_add_unique failing to replace existing value");

static int test_hash_iter(void)
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

	for (hash_iter_init(h, &iter); hash_iter_next(&iter, &k, (const void **)&v);) {
		v2 = hash_find(h2, k);
		TS_ASSERT(v2 != NULL);
		hash_del(h2, k);
	}

	TS_ASSERT(hash_get_count(h) == 3);
	TS_ASSERT(hash_get_count(h2) == 0);

	hash_free(h);
	hash_free(h2);
	return 0;
}
DEFINE_TEST(test_hash_iter, .description = "test hash_iter");

static int test_hash_iter_after_del(void)
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

	for (hash_iter_init(h, &iter); hash_iter_next(&iter, &k, (const void **)&v);) {
		v2 = hash_find(h2, k);
		TS_ASSERT(v2 != NULL);
		hash_del(h2, k);
	}

	TS_ASSERT(hash_get_count(h) == 2);
	TS_ASSERT(hash_get_count(h2) == 1);

	hash_free(h);
	hash_free(h2);
	return 0;
}
DEFINE_TEST(test_hash_iter_after_del,
	    .description = "test hash_iter, after deleting element");

static int test_hash_del(void)
{
	struct hash *h = hash_new(32, NULL);
	const char *k1 = "k1";
	const char *v1 = "v1";

	hash_add(h, k1, v1);
	hash_del(h, k1);

	hash_free(h);

	return 0;
}
DEFINE_TEST(test_hash_del, .description = "test add / delete a single element");

static int test_hash_del_nonexistent(void)
{
	struct hash *h = hash_new(32, NULL);
	const char *k1 = "k1";
	int rc;

	rc = hash_del(h, k1);
	TS_ASSERT(rc == -ENOENT);

	hash_free(h);

	return 0;
}
DEFINE_TEST(test_hash_del_nonexistent,
	    .description = "test deleting an element that doesn't exist");

static int test_hash_free(void)
{
	struct hash *h = hash_new(8, countfreecalls);
	const char *k1 = "k1", *k2 = "k2", *k3 = "k3";
	const char *v1 = "v1", *v2 = "v2", *v3 = "v3";

	hash_add(h, k1, v1);
	hash_add(h, k2, v2);
	hash_add(h, k3, v3);

	hash_del(h, k1);

	TS_ASSERT(freecount == 1);

	TS_ASSERT(hash_get_count(h) == 2);

	hash_free(h);

	TS_ASSERT(freecount == 3);

	return 0;
}
DEFINE_TEST(test_hash_free,
	    .description = "test hash_free calling free function for all values");

static int test_hash_add_unique(void)
{
	static const char *const k[] = { "k1", "k2", "k3", "k4", "k5" };
	static const char *const v[] = { "v1", "v2", "v3", "v4", "v5" };
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

		TS_ASSERT(hash_get_count(h) == N);
		hash_free(h);
	}
	return 0;
}
DEFINE_TEST(test_hash_add_unique,
	    .description = "test hash_add_unique with different key orders");

static int test_hash_massive_add_del(void)
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

	TS_ASSERT(hash_get_count(h) == N);

	k = &buf[0];
	for (i = 0; i < N; i++) {
		hash_del(h, k);
		k += 8;
	}

	TS_ASSERT(hash_get_count(h) == 0);

	hash_free(h);
	return 0;
}
DEFINE_TEST(test_hash_massive_add_del,
	    .description = "test multiple adds followed by multiple dels");

TESTSUITE_MAIN();
