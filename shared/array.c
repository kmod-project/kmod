// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2011-2013  ProFUSION embedded systems
 */

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <shared/array.h>

/* basic pointer array growing in steps */


static int array_realloc(struct array *array, size_t new_total)
{
	void *tmp;

	if (SIZE_MAX / sizeof(void *) < new_total)
		return -ENOMEM;
	tmp = realloc(array->array, sizeof(void *) * new_total);
	if (tmp == NULL)
		return -ENOMEM;
	array->array = tmp;
	array->total = new_total;
	return 0;
}

void array_init(struct array *array, size_t step)
{
	assert(step > 0);
	array->array = NULL;
	array->count = 0;
	array->total = 0;
	array->step = step;
}

int array_append(struct array *array, const void *element)
{
	size_t idx;

	if (array->count + 1 >= array->total) {
		int r;

		if (SIZE_MAX - array->total < array->step)
			return -ENOMEM;
		r = array_realloc(array, array->total + array->step);
		if (r < 0)
			return r;
	}
	idx = array->count;
	array->array[idx] = (void *)element;
	array->count++;
	return idx;
}

int array_append_unique(struct array *array, const void *element)
{
	void **itr = array->array;
	void **itr_end = itr + array->count;
	for (; itr < itr_end; itr++)
		if (*itr == element)
			return -EEXIST;
	return array_append(array, element);
}

void array_pop(struct array *array) {
	if (array->count == 0)
		return;
	array->count--;
	if (array->count + array->step < array->total) {
		int r = array_realloc(array, array->total - array->step);
		if (r < 0)
			return;
	}
}

void array_free_array(struct array *array) {
	free(array->array);
	array->array = NULL;
	array->count = 0;
	array->total = 0;
}


void array_sort(struct array *array, int (*cmp)(const void *a, const void *b))
{
	qsort(array->array, array->count, sizeof(void *), cmp);
}

int array_remove_at(struct array *array, size_t pos)
{
	if (array->count <= pos)
		return -ENOENT;

	array->count--;
	if (pos < array->count)
		memmove(array->array + pos, array->array + pos + 1,
			sizeof(void *) * (array->count - pos));

	if (array->count + array->step < array->total) {
		int r = array_realloc(array, array->total - array->step);
		/* ignore error */
		if (r < 0)
			return 0;
	}

	return 0;
}
