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

#include <shared/hash.h>
#include <shared/util.h>

#include "testsuite.h"

static int freecount;

static void countfreecalls(_maybe_unused_ void *v)
{
	freecount++;
}

static int test_hash_new(void)
{
	struct hash *h = hash_new(8, NULL);
	OK(h != NULL, "Cannot create hash table");
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

	OK(hash_get_count(h) == 3, "Wrong number of hash entries");

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

	OK(r == 0, "Failed to add hash entry(ies)");
	OK(hash_get_count(h) == 3, "Wrong number of hash entries");

	v = hash_find(h, "k1");
	OK(streq(v, v4), "Found incorrect hash entry");

	OK(freecount == 1, "Incorrect free hash entries");

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

	OK(r == 0, "Failed to add hash entry(ies)");

	/* replace v1 */
	r = hash_add_unique(h, k1, v4);
	OK(r != 0, "Failed to add unique hash entry");
	OK(hash_get_count(h) == 3, "Wrong number of hash entries");

	v = hash_find(h, "k1");
	OK(streq(v, v1), "Found incorrect hash entry");

	OK(freecount == 0, "Incorrect free hash entries");

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
		OK(v2 != NULL, "Cannot find hash entry");
		hash_del(h2, k);
	}

	OK(hash_get_count(h) == 3, "Wrong number of hash entries");
	OK(hash_get_count(h2) == 0, "Wrong number of hash entries");

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
		OK(v2 != NULL, "Cannot find hash entry");
		hash_del(h2, k);
	}

	OK(hash_get_count(h) == 2, "Wrong number of hash entries");
	OK(hash_get_count(h2) == 1, "Wrong number of hash entries");

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
	OK(rc == -ENOENT, "Incorrect ret code deleting non-existent entry");

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

	OK(freecount == 1, "Incorrect free hash entries");

	OK(hash_get_count(h) == 2, "Wrong number of hash entries");

	hash_free(h);

	OK(freecount == 3, "Incorrect free hash entries");

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

		OK(hash_get_count(h) == N, "Wrong number of hash entries");
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

	OK(hash_get_count(h) == N, "Wrong number of hash entries");

	k = &buf[0];
	for (i = 0; i < N; i++) {
		hash_del(h, k);
		k += 8;
	}

	OK(hash_get_count(h) == 0, "Wrong number of hash entries");

	hash_free(h);
	return 0;
}
DEFINE_TEST(test_hash_massive_add_del,
	    .description = "test multiple adds followed by multiple dels");

TESTSUITE_MAIN();
