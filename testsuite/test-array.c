// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C)  2014 Intel Corporation. All rights reserved.
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <shared/array.h>

#include "testsuite.h"

static int test_array_append1(void)
{
	struct array array;
	const char *c1 = "test1";

	array_init(&array, 2);
	array_append(&array, c1);
	OK(array.count == 1, "Incorrect number of array entries");
	OK(array.array[0] == c1, "Incorrect array entry");
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
	OK(array.count == 3, "Incorrect number of array entries");
	OK(array.array[0] == c1, "Incorrect array entry");
	OK(array.array[1] == c2, "Incorrect array entry");
	OK(array.array[2] == c3, "Incorrect array entry");
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
	OK(array.count == 3, "Incorrect number of array entries");
	OK(array.array[0] == c1, "Incorrect array entry");
	OK(array.array[1] == c2, "Incorrect array entry");
	OK(array.array[2] == c3, "Incorrect array entry");
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
	OK(array.count == 6, "Incorrect number of array entries");
	OK(array.array[0] == c1, "Incorrect array entry");
	OK(array.array[1] == c1, "Incorrect array entry");
	OK(array.array[2] == c2, "Incorrect array entry");
	OK(array.array[3] == c2, "Incorrect array entry");
	OK(array.array[4] == c3, "Incorrect array entry");
	OK(array.array[5] == c3, "Incorrect array entry");
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
	OK(array.count == 2, "Incorrect number of array entries");
	OK(array.array[0] == c1, "Incorrect array entry");
	OK(array.array[1] == c2, "Incorrect array entry");
	OK(array.total == 4, "Incorrect array size");

	array_remove_at(&array, 0);
	OK(array.count == 1, "Incorrect number of array entries");
	OK(array.array[0] == c2, "Incorrect array entry");
	OK(array.total == 2, "Incorrect array size");

	array_remove_at(&array, 0);
	OK(array.count == 0, "Incorrect number of array entries");
	OK(array.total == 2, "Incorrect array size");

	array_append(&array, c1);
	array_append(&array, c2);
	array_append(&array, c3);

	array_remove_at(&array, 1);
	OK(array.count == 2, "Incorrect number of array entries");
	OK(array.array[0] == c1, "Incorrect array entry");
	OK(array.array[1] == c3, "Incorrect array entry");
	OK(array.total == 4, "Incorrect array size");

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

	OK(array.count == 2, "Incorrect number of array entries");
	OK(array.array[0] == c1, "Incorrect array entry");
	OK(array.array[1] == c2, "Incorrect array entry");

	array_pop(&array);
	array_pop(&array);

	OK(array.count == 0, "Incorrect number of array entries");

	array_pop(&array);

	OK(array.count == 0, "Incorrect number of array entries");

	array_free_array(&array);

	return 0;
}

DEFINE_TEST(test_array_pop, .description = "test array pop");

TESTSUITE_MAIN();
