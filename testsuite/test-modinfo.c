// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2012-2013  ProFUSION embedded systems
 */

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "testsuite.h"

#define DEFINE_MODINFO_TEST(_field, _flavor, ...)                      \
	static int test_modinfo_##_field(void)                         \
	{                                                              \
		return EXEC_TOOL(modinfo, "-F", #_field, __VA_ARGS__); \
	}                                                              \
	DEFINE_TEST(test_modinfo_##_field, \
	.description = "check " #_field " output of modinfo for different architectures", \
	.config = { \
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modinfo/", \
	}, \
	.output = { \
		.out = TESTSUITE_ROOTFS "test-modinfo/correct-" #_field #_flavor ".txt", \
	})

/* TODO: add cross-compiled modules to the test */
#define DEFINE_MODINFO_GENERIC_TEST(_field) \
	DEFINE_MODINFO_TEST(_field, , "/mod-simple.ko")

#if ENABLE_OPENSSL
#define DEFINE_MODINFO_SIGN_TEST(_field)                             \
	DEFINE_MODINFO_TEST(_field, -openssl, "/mod-simple-sha1.ko", \
			    "/mod-simple-sha256.ko", "/mod-simple-pkcs7.ko")
#else
#define DEFINE_MODINFO_SIGN_TEST(_field)                                              \
	DEFINE_MODINFO_TEST(_field, , "/mod-simple-sha1.ko", "/mod-simple-sha256.ko", \
			    "/mod-simple-pkcs7.ko")
#endif

DEFINE_MODINFO_GENERIC_TEST(filename);
DEFINE_MODINFO_GENERIC_TEST(author);
DEFINE_MODINFO_GENERIC_TEST(license);
DEFINE_MODINFO_GENERIC_TEST(description);
DEFINE_MODINFO_GENERIC_TEST(parm);
DEFINE_MODINFO_GENERIC_TEST(depends);

DEFINE_MODINFO_SIGN_TEST(signer);
DEFINE_MODINFO_SIGN_TEST(sig_key);
DEFINE_MODINFO_SIGN_TEST(sig_hashalgo);

#if 0
static int test_modinfo_signature(void)
{
	return EXEC_TOOL(modinfo);
}
DEFINE_TEST(test_modinfo_signature,
	.description = "check signatures are correct for modinfo is correct for i686, ppc64, s390x and x86_64",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modinfo/",
	},
	.output = {
		.out = TESTSUITE_ROOTFS "test-modinfo/correct.txt",
	});
#endif

static int test_modinfo_external(void)
{
	return EXEC_TOOL(modinfo, "-F", "filename", "mod-simple");
}
DEFINE_TEST(test_modinfo_external,
	.description = "check if modinfo finds external module",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modinfo/external",
		[TC_UNAME_R] = "4.4.4",
	},
	.output = {
		.out = TESTSUITE_ROOTFS "test-modinfo/correct-external.txt",
	});

static int test_modinfo_builtin(void)
{
	return EXEC_TOOL(modinfo, "intel_uncore");
}
DEFINE_TEST(test_modinfo_builtin,
	.description = "check if modinfo finds builtin module",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modinfo/builtin",
		[TC_UNAME_R] = "6.11.0",
	},
	.output = {
		.out = TESTSUITE_ROOTFS "test-modinfo/correct-builtin.txt",
	});

TESTSUITE_MAIN();
