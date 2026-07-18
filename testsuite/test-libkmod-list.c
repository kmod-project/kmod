// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2015 Intel Corporation. All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>

#include <libkmod/libkmod-internal.h>

#include "testsuite.h"

static int len(struct kmod_list *list)
{
	int count = 0;
	struct kmod_list *l;
	kmod_list_foreach(l, list)
		count++;
	return count;
}

static void kmod_list_remove_all(struct kmod_list *list)
{
	while (list)
		list = kmod_list_remove(list);
}

static int test_list_last(void)
{
	struct kmod_list *list = NULL, *last;
	int i;
	static const char *const v[] = { "v1", "v2", "v3", "v4", "v5" };
	const int N = ARRAY_SIZE(v);

	for (i = 0; i < N; i++)
		list = kmod_list_append(list, v[i]);
	TS_ASSERT(len(list) == N);

	last = kmod_list_last(list);
	TS_ASSERT(last->data == v[N - 1]);

	kmod_list_remove_all(list);

	return 0;
}
DEFINE_TEST(test_list_last, .description = "test for the last element of a list");

static int test_list_prev(void)
{
	struct kmod_list *list = NULL, *l, *p;
	int i;
	static const char *const v[] = { "v1", "v2", "v3", "v4", "v5" };
	const int N = ARRAY_SIZE(v);

	l = kmod_list_prev(list, list);
	TS_ASSERT(l == NULL);

	for (i = 0; i < N; i++)
		list = kmod_list_append(list, v[i]);

	l = kmod_list_prev(list, list);
	TS_ASSERT(l == NULL);

	l = list;
	for (i = 0; i < N - 1; i++) {
		l = kmod_list_next(list, l);
		p = kmod_list_prev(list, l);
		TS_ASSERT(p->data == v[i]);
	}

	kmod_list_remove_all(list);

	return 0;
}
DEFINE_TEST(test_list_prev, .description = "test list prev");

static int test_list_remove_data(void)
{
	struct kmod_list *list = NULL, *l;
	int i;
	static const char *const v[] = { "v1", "v2", "v3", "v4", "v5" };
	const char *removed;
	const int N = ARRAY_SIZE(v);

	for (i = 0; i < N; i++)
		list = kmod_list_append(list, v[i]);

	removed = v[N / 2];
	list = kmod_list_remove_data(list, removed);
	TS_ASSERT(len(list) == N - 1);

	kmod_list_foreach(l, list)
		TS_ASSERT(l->data != removed);

	kmod_list_remove_all(list);

	return 0;
}
DEFINE_TEST(test_list_remove_data,
	    .description = "test list function to remove element by data");

static int test_list_append_list(void)
{
	struct kmod_list *a = NULL, *b = NULL, *c, *l;
	int i;
	static const char *const v[] = { "v1", "v2", "v3", "v4", "v5" };
	const int N = ARRAY_SIZE(v), M = N / 2;

	for (i = 0; i < M; i++)
		a = kmod_list_append(a, v[i]);
	TS_ASSERT(len(a) == M);

	for (i = M; i < N; i++)
		b = kmod_list_append(b, v[i]);
	TS_ASSERT(len(b) == N - M);

	a = kmod_list_append_list(a, NULL);
	TS_ASSERT(len(a) == M);

	b = kmod_list_append_list(NULL, b);
	TS_ASSERT(len(b) == N - M);

	c = kmod_list_append_list(a, b);
	TS_ASSERT(len(c) == N);

	i = 0;
	kmod_list_foreach(l, c) {
		TS_ASSERT(l->data == v[i]);
		i++;
	}

	kmod_list_remove_all(c);

	return 0;
}
DEFINE_TEST(test_list_append_list,
	    .description = "test list function to append another list");

static int test_list_insert_before(void)
{
	struct kmod_list *list = NULL, *l;
	const char *v1 = "v1", *v2 = "v2", *v3 = "v3", *vx = "vx";

	list = kmod_list_insert_before(list, v3);
	TS_ASSERT(len(list) == 1);

	list = kmod_list_insert_before(list, v2);
	list = kmod_list_insert_before(list, v1);
	TS_ASSERT(len(list) == 3);

	l = list;
	TS_ASSERT(l->data == v1);

	l = kmod_list_next(list, l);
	TS_ASSERT(l->data == v2);

	l = kmod_list_insert_before(l, vx);
	TS_ASSERT(len(list) == 4);
	TS_ASSERT(l->data == vx);

	l = kmod_list_next(list, l);
	TS_ASSERT(l->data == v2);

	l = kmod_list_next(list, l);
	TS_ASSERT(l->data == v3);

	kmod_list_remove_all(list);

	return 0;
}
DEFINE_TEST(test_list_insert_before,
	    .description = "test list function to insert before element");

static int test_list_insert_after(void)
{
	struct kmod_list *list = NULL, *l;
	const char *v1 = "v1", *v2 = "v2", *v3 = "v3", *vx = "vx";

	list = kmod_list_insert_after(list, v1);
	TS_ASSERT(len(list) == 1);

	list = kmod_list_insert_after(list, v3);
	list = kmod_list_insert_after(list, v2);
	TS_ASSERT(len(list) == 3);

	l = list;
	TS_ASSERT(l->data == v1);

	l = kmod_list_insert_after(l, vx);
	TS_ASSERT(len(list) == 4);
	TS_ASSERT(l->data == v1);

	l = kmod_list_next(list, l);
	TS_ASSERT(l->data == vx);

	l = kmod_list_next(list, l);
	TS_ASSERT(l->data == v2);

	l = kmod_list_next(list, l);
	TS_ASSERT(l->data == v3);

	kmod_list_remove_all(list);

	return 0;
}
DEFINE_TEST(test_list_insert_after,
	    .description = "test list function to insert after element");

TESTSUITE_MAIN();
