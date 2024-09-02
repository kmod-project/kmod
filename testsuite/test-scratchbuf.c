// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C)  2016 Intel Corporation. All rights reserved.
 */

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <shared/scratchbuf.h>

#include "testsuite.h"

static int test_scratchbuf_onlystack(const struct test *t)
{
	struct scratchbuf sbuf;
	const char *smallstr = "xyz";
	char buf[3 + 2];
	char buf2[3 + 1];

	scratchbuf_init(&sbuf, buf, sizeof(buf));
	assert_return(scratchbuf_alloc(&sbuf, strlen(smallstr) + 1) == 0, EXIT_FAILURE);
	assert_return(sbuf.need_free == false, EXIT_FAILURE);
	scratchbuf_release(&sbuf);

	scratchbuf_init(&sbuf, buf2, sizeof(buf2));
	assert_return(scratchbuf_alloc(&sbuf, strlen(smallstr) + 1) == 0, EXIT_FAILURE);
	assert_return(sbuf.need_free == false, EXIT_FAILURE);
	scratchbuf_release(&sbuf);

	memcpy(scratchbuf_str(&sbuf), smallstr, strlen(smallstr) + 1);
	assert_return(strcmp(scratchbuf_str(&sbuf), smallstr) == 0, EXIT_FAILURE);

	return 0;
}
DEFINE_TEST(test_scratchbuf_onlystack,
	    .description = "test scratchbuf for buffer on stack only");

static int test_scratchbuf_heap(const struct test *t)
{
	struct scratchbuf sbuf;
	const char *smallstr = "xyz";
	const char *largestr = "xyzxyzxyz";
	const char *largestr2 = "xyzxyzxyzxyzxyz";
	char buf[3 + 1];

	scratchbuf_init(&sbuf, buf, sizeof(buf));

	/* Initially only on stack */
	assert_return(scratchbuf_alloc(&sbuf, strlen(smallstr) + 1) == 0, EXIT_FAILURE);
	assert_return(sbuf.need_free == false, EXIT_FAILURE);
	memcpy(scratchbuf_str(&sbuf), smallstr, strlen(smallstr) + 1);

	/* Grow once to heap */
	assert_return(scratchbuf_alloc(&sbuf, strlen(largestr) + 1) == 0, EXIT_FAILURE);
	assert_return(sbuf.need_free == true, EXIT_FAILURE);
	assert_return(sbuf.size == strlen(largestr) + 1, EXIT_FAILURE);
	assert_return(strcmp(scratchbuf_str(&sbuf), smallstr) == 0, EXIT_FAILURE);
	memcpy(scratchbuf_str(&sbuf), largestr, strlen(largestr) + 1);
	assert_return(strcmp(scratchbuf_str(&sbuf), largestr) == 0, EXIT_FAILURE);

	/* Grow again - realloc should take place */
	assert_return(scratchbuf_alloc(&sbuf, strlen(largestr2) + 1) == 0, EXIT_FAILURE);
	assert_return(sbuf.need_free == true, EXIT_FAILURE);
	assert_return(sbuf.size == strlen(largestr2) + 1, EXIT_FAILURE);
	memcpy(scratchbuf_str(&sbuf), largestr2, strlen(largestr2) + 1);
	assert_return(strcmp(scratchbuf_str(&sbuf), largestr2) == 0, EXIT_FAILURE);

	scratchbuf_release(&sbuf);

	return 0;
}
DEFINE_TEST(test_scratchbuf_heap,
	    .description = "test scratchbuf for buffer on that grows to heap");

TESTSUITE_MAIN();
