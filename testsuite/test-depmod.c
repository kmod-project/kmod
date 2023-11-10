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

#define MODULES_UNAME "4.4.4"
#define MODULES_ORDER_ROOTFS TESTSUITE_ROOTFS "test-depmod/modules-order-compressed"
#define MODULES_ORDER_LIB_MODULES MODULES_ORDER_ROOTFS MODULE_DIRECTORY "/" MODULES_UNAME
static noreturn int depmod_modules_order_for_compressed(const struct test *t)
{
	const char *progname = ABS_TOP_BUILDDIR "/tools/depmod";
	const char *const args[] = {
		progname,
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}

DEFINE_TEST(depmod_modules_order_for_compressed,
	.description = "check if depmod let aliases in right order when using compressed modules",
	.config = {
		[TC_UNAME_R] = MODULES_UNAME,
		[TC_ROOTFS] = MODULES_ORDER_ROOTFS,
	},
	.output = {
		.files = (const struct keyval[]) {
			{ MODULES_ORDER_LIB_MODULES "/correct-modules.alias",
			  MODULES_ORDER_LIB_MODULES "/modules.alias" },
			{ }
		},
	});

#define MODULES_OUTDIR_ROOTFS TESTSUITE_ROOTFS "test-depmod/modules-outdir"
#define MODULES_OUTDIR_LIB_MODULES_OUTPUT MODULES_OUTDIR_ROOTFS "/outdir" MODULE_DIRECTORY "/" MODULES_UNAME
#define MODULES_OUTDIR_LIB_MODULES_INPUT MODULES_OUTDIR_ROOTFS MODULE_DIRECTORY "/" MODULES_UNAME
static noreturn int depmod_modules_outdir(const struct test *t)
{
	const char *progname = ABS_TOP_BUILDDIR "/tools/depmod";
	const char *const args[] = {
		progname,
		"--outdir", MODULES_OUTDIR_ROOTFS "/outdir/",
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}

DEFINE_TEST(depmod_modules_outdir,
	.description = "check if depmod honours the outdir option",
	.config = {
		[TC_UNAME_R] = MODULES_UNAME,
		[TC_ROOTFS] = MODULES_OUTDIR_ROOTFS,
	},
	.output = {
		.files = (const struct keyval[]) {
			{ MODULES_OUTDIR_LIB_MODULES_OUTPUT "/modules.dep",
			  MODULES_OUTDIR_ROOTFS "/correct-modules.dep" },
			{ MODULES_OUTDIR_LIB_MODULES_OUTPUT "/modules.alias",
			  MODULES_OUTDIR_ROOTFS "/correct-modules.alias" },
			{ }
		},
	});

#define SEARCH_ORDER_SIMPLE_ROOTFS TESTSUITE_ROOTFS "test-depmod/search-order-simple"
#define SEARCH_ORDER_SIMPLE_LIB_MODULES SEARCH_ORDER_SIMPLE_ROOTFS MODULE_DIRECTORY "/" MODULES_UNAME
static noreturn int depmod_search_order_simple(const struct test *t)
{
	const char *progname = ABS_TOP_BUILDDIR "/tools/depmod";
	const char *const args[] = {
		progname,
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(depmod_search_order_simple,
	.description = "check if depmod honor search order in config",
	.config = {
		[TC_UNAME_R] = MODULES_UNAME,
		[TC_ROOTFS] = SEARCH_ORDER_SIMPLE_ROOTFS,
	},
	.output = {
		.files = (const struct keyval[]) {
			{ SEARCH_ORDER_SIMPLE_LIB_MODULES "/correct-modules.dep",
			  SEARCH_ORDER_SIMPLE_LIB_MODULES "/modules.dep" },
			{ }
		},
	});

#define SEARCH_ORDER_SAME_PREFIX_ROOTFS TESTSUITE_ROOTFS "test-depmod/search-order-same-prefix"
#define SEARCH_ORDER_SAME_PREFIX_LIB_MODULES SEARCH_ORDER_SAME_PREFIX_ROOTFS MODULE_DIRECTORY "/" MODULES_UNAME
static noreturn int depmod_search_order_same_prefix(const struct test *t)
{
	const char *progname = ABS_TOP_BUILDDIR "/tools/depmod";
	const char *const args[] = {
		progname,
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(depmod_search_order_same_prefix,
	.description = "check if depmod honor search order in config with same prefix",
	.config = {
		[TC_UNAME_R] = MODULES_UNAME,
		[TC_ROOTFS] = SEARCH_ORDER_SAME_PREFIX_ROOTFS,
	},
	.output = {
		.files = (const struct keyval[]) {
			{ SEARCH_ORDER_SAME_PREFIX_LIB_MODULES "/correct-modules.dep",
			  SEARCH_ORDER_SAME_PREFIX_LIB_MODULES "/modules.dep" },
			{ }
		},
	});

#define DETECT_LOOP_ROOTFS TESTSUITE_ROOTFS "test-depmod/detect-loop"
static noreturn int depmod_detect_loop(const struct test *t)
{
	const char *progname = ABS_TOP_BUILDDIR "/tools/depmod";
	const char *const args[] = {
		progname,
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(depmod_detect_loop,
	.description = "check if depmod detects module loops correctly",
	.config = {
		[TC_UNAME_R] = MODULES_UNAME,
		[TC_ROOTFS] = DETECT_LOOP_ROOTFS,
	},
	.expected_fail = true,
	.output = {
		.err = DETECT_LOOP_ROOTFS "/correct.txt",
	});

#define SEARCH_ORDER_EXTERNAL_FIRST_ROOTFS TESTSUITE_ROOTFS "test-depmod/search-order-external-first"
#define SEARCH_ORDER_EXTERNAL_FIRST_LIB_MODULES SEARCH_ORDER_EXTERNAL_FIRST_ROOTFS MODULE_DIRECTORY "/" MODULES_UNAME
static noreturn int depmod_search_order_external_first(const struct test *t)
{
	const char *progname = ABS_TOP_BUILDDIR "/tools/depmod";
	const char *const args[] = {
		progname,
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(depmod_search_order_external_first,
	.description = "check if depmod honor external keyword with higher priority",
	.config = {
		[TC_UNAME_R] = MODULES_UNAME,
		[TC_ROOTFS] = SEARCH_ORDER_EXTERNAL_FIRST_ROOTFS,
	},
	.output = {
		.files = (const struct keyval[]) {
			{ SEARCH_ORDER_EXTERNAL_FIRST_LIB_MODULES "/correct-modules.dep",
			  SEARCH_ORDER_EXTERNAL_FIRST_LIB_MODULES "/modules.dep" },
			{ }
		},
	});

#define SEARCH_ORDER_EXTERNAL_LAST_ROOTFS TESTSUITE_ROOTFS "test-depmod/search-order-external-last"
#define SEARCH_ORDER_EXTERNAL_LAST_LIB_MODULES SEARCH_ORDER_EXTERNAL_LAST_ROOTFS MODULE_DIRECTORY "/" MODULES_UNAME
static noreturn int depmod_search_order_external_last(const struct test *t)
{
	const char *progname = ABS_TOP_BUILDDIR "/tools/depmod";
	const char *const args[] = {
		progname,
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(depmod_search_order_external_last,
	.description = "check if depmod honor external keyword with lower priority",
	.config = {
		[TC_UNAME_R] = MODULES_UNAME,
		[TC_ROOTFS] = SEARCH_ORDER_EXTERNAL_LAST_ROOTFS,
	},
	.output = {
		.files = (const struct keyval[]) {
			{ SEARCH_ORDER_EXTERNAL_LAST_LIB_MODULES "/correct-modules.dep",
			  SEARCH_ORDER_EXTERNAL_LAST_LIB_MODULES "/modules.dep" },
			{ }
		},
	});

#define SEARCH_ORDER_OVERRIDE_ROOTFS TESTSUITE_ROOTFS "test-depmod/search-order-override"
#define SEARCH_ORDER_OVERRIDE_LIB_MODULES SEARCH_ORDER_OVERRIDE_ROOTFS MODULE_DIRECTORY "/" MODULES_UNAME
static noreturn int depmod_search_order_override(const struct test *t)
{
	const char *progname = ABS_TOP_BUILDDIR "/tools/depmod";
	const char *const args[] = {
		progname,
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(depmod_search_order_override,
	.description = "check if depmod honor override keyword",
	.config = {
		[TC_UNAME_R] = MODULES_UNAME,
		[TC_ROOTFS] = SEARCH_ORDER_OVERRIDE_ROOTFS,
	},
	.output = {
		.files = (const struct keyval[]) {
			{ SEARCH_ORDER_OVERRIDE_LIB_MODULES "/correct-modules.dep",
			  SEARCH_ORDER_OVERRIDE_LIB_MODULES "/modules.dep" },
			{ }
		},
	});

TESTSUITE_MAIN();
