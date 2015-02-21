/*
 * Copyright (C) 2012-2013  ProFUSION embedded systems
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
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "testsuite.h"

static const char *progname = ABS_TOP_BUILDDIR "/tools/modinfo";

#define DEFINE_MODINFO_TEST(_field) \
static noreturn int test_modinfo_##_field(const struct test *t) \
{ \
	const char *const args[] = { \
		progname, "-F", #_field ,\
		"/mod-simple-i386.ko", "/mod-simple-x86_64.ko", "/mod-simple-sparc64.ko", \
		NULL, \
	}; \
	test_spawn_prog(progname, args); \
	exit(EXIT_FAILURE); \
} \
DEFINE_TEST(test_modinfo_##_field, \
	.description = "check " #_field " output of modinfo for different architectures", \
	.config = { \
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modinfo/", \
	}, \
	.output = { \
		.out = TESTSUITE_ROOTFS "test-modinfo/correct-" #_field ".txt", \
	})

DEFINE_MODINFO_TEST(filename);
DEFINE_MODINFO_TEST(author);
DEFINE_MODINFO_TEST(license);
DEFINE_MODINFO_TEST(description);
DEFINE_MODINFO_TEST(parm);
DEFINE_MODINFO_TEST(depends);

static noreturn int test_modinfo_signature(const struct test *t)
{
	const char *const args[] = {
		progname,
		"/ext4-x86_64-sha1.ko", "/ext4-x86_64-sha256.ko",
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(test_modinfo_signature,
	.description = "check if output for modinfo is correct for i686, ppc64, s390x and x86_64",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modinfo/",
	},
	.output = {
		.out = TESTSUITE_ROOTFS "test-modinfo/correct.txt",
	});

TESTSUITE_MAIN();
