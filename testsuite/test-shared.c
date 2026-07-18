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
#include <shared/strbuf.h>
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

/* strbuf sub-group */

static const char *TEXT =
	"this is a very long test that is longer than the size we initially se in the strbuf";

static int test_strbuf_pushchar(void)
{
	_cleanup_strbuf_ struct strbuf buf;
	const char *result;
	const char *c;

	strbuf_init(&buf);

	for (c = TEXT; *c != '\0'; c++)
		strbuf_pushchar(&buf, *c);

	result = strbuf_str(&buf);
	TS_ASSERT(result == buf.bytes);
	TS_ASSERT(streq(result, TEXT));

	return 0;
}
DEFINE_TEST(test_strbuf_pushchar, .description = "test strbuf_{pushchar, str, steal}");

static int test_strbuf_pushchars(void)
{
	_cleanup_strbuf_ struct strbuf buf;
	const char *result;
	char *saveptr = NULL, *str;
	const char *c;
	size_t lastwordlen = 0;

	strbuf_init(&buf);
	str = strdup(TEXT);
	for (c = strtok_r(str, " ", &saveptr); c != NULL;
	     c = strtok_r(NULL, " ", &saveptr)) {
		strbuf_pushchars(&buf, c);
		strbuf_pushchar(&buf, ' ');
		lastwordlen = strlen(c);
	}

	/*
	 * Replace the last space char, which also guarantees there's at least 1 char
	 * available for the '\0' added by strbuf_str() so result1 == buf.bytes should be
	 * true
	 */
	strbuf_popchar(&buf);
	result = strbuf_str(&buf);
	TS_ASSERT(result == buf.bytes);
	TS_ASSERT(streq(result, TEXT));

	strbuf_popchars(&buf, lastwordlen);
	result = strbuf_str(&buf);
	TS_ASSERT(!streq(TEXT, result));
	TS_ASSERT(strncmp(TEXT, result, strlen(TEXT) - lastwordlen) == 0);
	TS_ASSERT(result[strlen(TEXT) - lastwordlen] == '\0');

	free(str);

	return 0;
}
DEFINE_TEST(test_strbuf_pushchars,
	    .description = "test strbuf_{pushchars, popchar, popchars}");

static int test_strbuf_with_stack(void)
{
	const char test[] = "test-something-small";
	const char *stack_buf;
	const char *p;
	DECLARE_STRBUF_WITH_STACK(buf, 256);
	DECLARE_STRBUF_WITH_STACK(buf2, sizeof(test) + 1);
	DECLARE_STRBUF_WITH_STACK(buf3, sizeof(test) + 1);

	strbuf_pushchars(&buf, test);
	TS_ASSERT(streq(test, strbuf_str(&buf)));
	p = strbuf_str(&buf);
	TS_ASSERT(streq(test, p));

	strbuf_pushchars(&buf2, test);
	TS_ASSERT(streq(test, strbuf_str(&buf2)));
	/* It fits on stack, but when we steal, we get a copy on heap */
	p = strbuf_str(&buf2);
	TS_ASSERT(streq(test, p));

	/*
	 * Check assumption about buffer being on stack vs heap is indeed valid.
	 * Not to be done in real code.
	 */
	strbuf_clear(&buf3);
	stack_buf = buf3.bytes;
	strbuf_pushchars(&buf3, test);
	TS_ASSERT(stack_buf == buf3.bytes);

	TS_ASSERT(streq(test, strbuf_str(&buf3)));
	TS_ASSERT(stack_buf == buf3.bytes);

	strbuf_pushchars(&buf3, "-overflow");
	TS_ASSERT(stack_buf != buf3.bytes);

	return 0;
}
DEFINE_TEST(test_strbuf_with_stack, .description = "test strbuf with stack");

static int test_strbuf_with_heap(void)
{
	DECLARE_STRBUF(heapbuf);

	TS_ASSERT(heapbuf.bytes == NULL);
	TS_ASSERT(heapbuf.size == 0);
	TS_ASSERT(heapbuf.used == 0);
	strbuf_pushchars(&heapbuf, "-overflow");
	TS_ASSERT(heapbuf.bytes != NULL);
	TS_ASSERT(heapbuf.size != 0);
	TS_ASSERT(heapbuf.used != 0);

	return 0;
}
DEFINE_TEST(test_strbuf_with_heap, .description = "test strbuf with heap only");

static int test_strbuf_pushmem(void)
{
	_cleanup_strbuf_ struct strbuf buf;

	strbuf_init(&buf);
	strbuf_pushmem(&buf, "", 0);
	strbuf_pushmem(&buf, TEXT, strlen(TEXT) + 1);

	TS_ASSERT(streq(TEXT, strbuf_str(&buf)));

	return 0;
}
DEFINE_TEST(test_strbuf_pushmem, .description = "test strbuf_reserve");

static int test_strbuf_used(void)
{
	_cleanup_strbuf_ struct strbuf buf;

	strbuf_init(&buf);
	TS_ASSERT(strbuf_used(&buf) == 0);

	strbuf_pushchars(&buf, TEXT);
	TS_ASSERT(strbuf_used(&buf) == strlen(TEXT));

	strbuf_pushchar(&buf, 'a');
	strbuf_popchar(&buf);
	TS_ASSERT(strbuf_used(&buf) == strlen(TEXT));

	TS_ASSERT(streq(TEXT, strbuf_str(&buf)));
	TS_ASSERT(strbuf_used(&buf) == strlen(TEXT));

	strbuf_pushchar(&buf, '\0');
	TS_ASSERT(streq(TEXT, strbuf_str(&buf)));
	TS_ASSERT(strbuf_used(&buf) == strlen(TEXT) + 1);

	return 0;
}
DEFINE_TEST(test_strbuf_used, .description = "test strbuf_used");

static int test_strbuf_shrink_to(void)
{
	_cleanup_strbuf_ struct strbuf buf;

	strbuf_init(&buf);
	strbuf_shrink_to(&buf, 0);
	TS_ASSERT(strbuf_used(&buf) == 0);

	strbuf_pushchars(&buf, TEXT);
	strbuf_shrink_to(&buf, strlen(TEXT) - 1);
	TS_ASSERT(strbuf_used(&buf) == strlen(TEXT) - 1);

	return 0;
}
DEFINE_TEST(test_strbuf_shrink_to, .description = "test strbuf_shrink_to");

static int xfail_strbuf_shrink_to(void)
{
	_cleanup_strbuf_ struct strbuf buf;

	strbuf_init(&buf);
	strbuf_pushchar(&buf, '/');

	/* This should crash on assert */
	strbuf_shrink_to(&buf, 2);

	return 0;
}
DEFINE_TEST(xfail_strbuf_shrink_to, .description = "xfail strbuf_shrink_to",
	    .expected_fail = true);

TESTSUITE_MAIN();
