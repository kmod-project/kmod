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

#include <shared/strbuf.h>
#include <shared/util.h>

#include "testsuite.h"

static const char *TEXT =
	"this is a very long test that is longer than the size we initially se in the strbuf";

static int test_strbuf_pushchar(const struct test *t)
{
	struct strbuf buf;
	char *result1, *result2;
	const char *c;

	strbuf_init(&buf);

	for (c = TEXT; *c != '\0'; c++)
		strbuf_pushchar(&buf, *c);

	result1 = (char *) strbuf_str(&buf);
	assert_return(result1 == buf.bytes, EXIT_FAILURE);
	assert_return(streq(result1, TEXT), EXIT_FAILURE);
	result1 = strdup(result1);

	result2 = strbuf_steal(&buf);
	assert_return(streq(result1, result2), EXIT_FAILURE);

	free(result1);
	free(result2);

	return 0;
}
DEFINE_TEST(test_strbuf_pushchar,
		.description = "test strbuf_{pushchar, str, steal}");

static int test_strbuf_pushchars(const struct test *t)
{
	struct strbuf buf;
	char *result1, *saveptr = NULL, *str, *result2;
	const char *c;
	int lastwordlen = 0;

	strbuf_init(&buf);
	str = strdup(TEXT);
	for (c = strtok_r(str, " ", &saveptr); c != NULL;
	     c = strtok_r(NULL, " ", &saveptr)) {
		strbuf_pushchars(&buf, c);
		strbuf_pushchar(&buf, ' ');
		lastwordlen = strlen(c);
	}

	strbuf_popchar(&buf);
	result1 = (char *) strbuf_str(&buf);
	assert_return(result1 == buf.bytes, EXIT_FAILURE);
	assert_return(streq(result1, TEXT), EXIT_FAILURE);

	strbuf_popchars(&buf, lastwordlen);
	result2 = strbuf_steal(&buf);
	assert_return(!streq(TEXT, result2), EXIT_FAILURE);
	assert_return(strncmp(TEXT, result2, strlen(TEXT) - lastwordlen) == 0,
		      EXIT_FAILURE);
	assert_return(result2[strlen(TEXT) - lastwordlen] == '\0',
		      EXIT_FAILURE);

	free(str);
	free(result2);

	return 0;
}
DEFINE_TEST(test_strbuf_pushchars,
		.description = "test strbuf_{pushchars, popchar, popchars}");


TESTSUITE_MAIN();
