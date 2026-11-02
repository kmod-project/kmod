// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Kron Kastrati <kronkastrati900@gmail.com>

#include <stdlib.h>
#include <string.h>

#include <libkmod/libkmod-internal-signature.h>

#include "testsuite.h"

static int test_hash_algo_names(void)
{
	for (unsigned int i = 0; i < HASH_ALGO__LAST; i++)
		TS_ASSERT(pkey_hash_algo[i] != NULL);

	return 0;
}
DEFINE_TEST(test_hash_algo_names,
	    .description = "test every kernel hash algo id has a name");

static int test_hash_algo_sm3_rmd128(void)
{
	TS_ASSERT(HASH_ALGO_SM3_256 == 17);
	TS_ASSERT(strcmp(pkey_hash_algo[HASH_ALGO_SM3_256], "sm3") == 0);
	TS_ASSERT(strcmp(pkey_hash_algo[HASH_ALGO_RIPE_MD_128], "rmd128") == 0);

	return 0;
}
DEFINE_TEST(test_hash_algo_sm3_rmd128,
	    .description = "test sm3 and rmd128 are at their kernel ids");

TESTSUITE_MAIN();
