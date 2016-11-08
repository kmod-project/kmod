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

#include <shared/array.h>

#include "testsuite.h"

static int test_array_append1(const struct test *t)
{
	struct array array;
	const char *c1 = "test1";

	array_init(&array, 2);
	array_append(&array, c1);
	assert_return(array.count == 1, EXIT_FAILURE);
	assert_return(array.array[0] == c1, EXIT_FAILURE);
	array_free_array(&array);

	return 0;
}
DEFINE_TEST(test_array_append1,
		.description = "test simple array append");


static int test_array_append2(const struct test *t)
{
	struct array array;
	const char *c1 = "test1";
	const char *c2 = "test2";
	const char *c3 = "test3";

	array_init(&array, 2);
	array_append(&array, c1);
	array_append(&array, c2);
	array_append(&array, c3);
	assert_return(array.count == 3, EXIT_FAILURE);
	assert_return(array.array[0] == c1, EXIT_FAILURE);
	assert_return(array.array[1] == c2, EXIT_FAILURE);
	assert_return(array.array[2] == c3, EXIT_FAILURE);
	array_free_array(&array);

	return 0;
}
DEFINE_TEST(test_array_append2,
		.description = "test array append over step");

static int test_array_append_unique(const struct test *t)
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
	assert_return(array.count == 3, EXIT_FAILURE);
	assert_return(array.array[0] == c1, EXIT_FAILURE);
	assert_return(array.array[1] == c2, EXIT_FAILURE);
	assert_return(array.array[2] == c3, EXIT_FAILURE);
	array_free_array(&array);

	return 0;
}
DEFINE_TEST(test_array_append_unique,
		.description = "test array append unique");

static int strptrcmp(const void *pa, const void *pb) {
	const char *a = *(const char **)pa;
	const char *b = *(const char **)pb;

	return strcmp(a, b);
}

static int test_array_sort(const struct test *t)
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
	assert_return(array.count == 6, EXIT_FAILURE);
	assert_return(array.array[0] == c1, EXIT_FAILURE);
	assert_return(array.array[1] == c1, EXIT_FAILURE);
	assert_return(array.array[2] == c2, EXIT_FAILURE);
	assert_return(array.array[3] == c2, EXIT_FAILURE);
	assert_return(array.array[4] == c3, EXIT_FAILURE);
	assert_return(array.array[5] == c3, EXIT_FAILURE);
	array_free_array(&array);

	return 0;
}
DEFINE_TEST(test_array_sort,
		.description = "test array sort");

static int test_array_remove_at(const struct test *t)
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
	assert_return(array.count == 2, EXIT_FAILURE);
	assert_return(array.array[0] == c1, EXIT_FAILURE);
	assert_return(array.array[1] == c2, EXIT_FAILURE);

	array_remove_at(&array, 0);
	assert_return(array.count == 1, EXIT_FAILURE);
	assert_return(array.array[0] == c2, EXIT_FAILURE);

	array_remove_at(&array, 0);
	assert_return(array.count == 0, EXIT_FAILURE);

	array_append(&array, c1);
	array_append(&array, c2);
	array_append(&array, c3);

	array_remove_at(&array, 1);
	assert_return(array.count == 2, EXIT_FAILURE);
	assert_return(array.array[0] == c1, EXIT_FAILURE);
	assert_return(array.array[1] == c3, EXIT_FAILURE);

	array_free_array(&array);

	return 0;
}
DEFINE_TEST(test_array_remove_at,
		.description = "test array remove at");

static int test_array_pop(const struct test *t)
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

	assert_return(array.count == 2, EXIT_FAILURE);
	assert_return(array.array[0] == c1, EXIT_FAILURE);
	assert_return(array.array[1] == c2, EXIT_FAILURE);

	array_pop(&array);
	array_pop(&array);

	assert_return(array.count == 0, EXIT_FAILURE);

	array_free_array(&array);

	return 0;
}

DEFINE_TEST(test_array_pop,
		.description = "test array pop");

TESTSUITE_MAIN();
