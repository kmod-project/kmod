// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C)  2014 Intel Corporation. All rights reserved.
 */

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

static int test_strbuf_pushchar(void)
{
	_cleanup_strbuf_ struct strbuf buf;
	const char *result;
	const char *c;

	strbuf_init(&buf);

	for (c = TEXT; *c != '\0'; c++)
		strbuf_pushchar(&buf, *c);

	result = strbuf_str(&buf);
	assert_return(result == buf.bytes, EXIT_FAILURE);
	assert_return(streq(result, TEXT), EXIT_FAILURE);

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
	assert_return(result == buf.bytes, EXIT_FAILURE);
	assert_return(streq(result, TEXT), EXIT_FAILURE);

	strbuf_popchars(&buf, lastwordlen);
	result = strbuf_str(&buf);
	assert_return(!streq(TEXT, result), EXIT_FAILURE);
	assert_return(strncmp(TEXT, result, strlen(TEXT) - lastwordlen) == 0,
		      EXIT_FAILURE);
	assert_return(result[strlen(TEXT) - lastwordlen] == '\0', EXIT_FAILURE);

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
	assert_return(streq(test, strbuf_str(&buf)), EXIT_FAILURE);
	p = strbuf_str(&buf);
	assert_return(streq(test, p), EXIT_FAILURE);

	strbuf_pushchars(&buf2, test);
	assert_return(streq(test, strbuf_str(&buf2)), EXIT_FAILURE);
	/* It fits on stack, but when we steal, we get a copy on heap */
	p = strbuf_str(&buf2);
	assert_return(streq(test, p), EXIT_FAILURE);

	/*
	 * Check assumption about buffer being on stack vs heap is indeed valid.
	 * Not to be done in real code.
	 */
	strbuf_clear(&buf3);
	stack_buf = buf3.bytes;
	strbuf_pushchars(&buf3, test);
	assert_return(stack_buf == buf3.bytes, EXIT_FAILURE);

	assert_return(streq(test, strbuf_str(&buf3)), EXIT_FAILURE);
	assert_return(stack_buf == buf3.bytes, EXIT_FAILURE);

	strbuf_pushchars(&buf3, "-overflow");
	assert_return(stack_buf != buf3.bytes, EXIT_FAILURE);

	return 0;
}
DEFINE_TEST(test_strbuf_with_stack, .description = "test strbuf with stack");

static int test_strbuf_with_heap(void)
{
	DECLARE_STRBUF(heapbuf);

	assert_return(heapbuf.bytes == NULL, EXIT_FAILURE);
	assert_return(heapbuf.size == 0, EXIT_FAILURE);
	assert_return(heapbuf.used == 0, EXIT_FAILURE);
	strbuf_pushchars(&heapbuf, "-overflow");
	assert_return(heapbuf.bytes != NULL, EXIT_FAILURE);
	assert_return(heapbuf.size != 0, EXIT_FAILURE);
	assert_return(heapbuf.used != 0, EXIT_FAILURE);

	return 0;
}
DEFINE_TEST(test_strbuf_with_heap, .description = "test strbuf with heap only");

static int test_strbuf_pushmem(void)
{
	_cleanup_strbuf_ struct strbuf buf;

	strbuf_init(&buf);
	strbuf_pushmem(&buf, "", 0);
	strbuf_pushmem(&buf, TEXT, strlen(TEXT) + 1);

	assert_return(streq(TEXT, strbuf_str(&buf)), EXIT_FAILURE);

	return 0;
}
DEFINE_TEST(test_strbuf_pushmem, .description = "test strbuf_reserve");

static int test_strbuf_used(void)
{
	_cleanup_strbuf_ struct strbuf buf;

	strbuf_init(&buf);
	assert_return(strbuf_used(&buf) == 0, EXIT_FAILURE);

	strbuf_pushchars(&buf, TEXT);
	assert_return(strbuf_used(&buf) == strlen(TEXT), EXIT_FAILURE);

	strbuf_pushchar(&buf, 'a');
	strbuf_popchar(&buf);
	assert_return(strbuf_used(&buf) == strlen(TEXT), EXIT_FAILURE);

	assert_return(streq(TEXT, strbuf_str(&buf)), EXIT_FAILURE);
	assert_return(strbuf_used(&buf) == strlen(TEXT), EXIT_FAILURE);

	strbuf_pushchar(&buf, '\0');
	assert_return(streq(TEXT, strbuf_str(&buf)), EXIT_FAILURE);
	assert_return(strbuf_used(&buf) == strlen(TEXT) + 1, EXIT_FAILURE);

	return 0;
}
DEFINE_TEST(test_strbuf_used, .description = "test strbuf_used");

static int test_strbuf_shrink_to(void)
{
	_cleanup_strbuf_ struct strbuf buf;

	strbuf_init(&buf);
	strbuf_shrink_to(&buf, 0);
	assert_return(strbuf_used(&buf) == 0, EXIT_FAILURE);

	strbuf_pushchars(&buf, TEXT);
	strbuf_shrink_to(&buf, strlen(TEXT) - 1);
	assert_return(strbuf_used(&buf) == strlen(TEXT) - 1, EXIT_FAILURE);

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
