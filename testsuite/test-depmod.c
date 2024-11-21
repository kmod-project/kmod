// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2012-2013  ProFUSION embedded systems
 */

#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "testsuite.h"

#define EXEC_DEPMOD(...)                     \
	test_spawn_prog(TOOLS_DIR "/depmod", \
			(const char *[]){ TOOLS_DIR "/depmod", ##__VA_ARGS__, NULL })

static noreturn int depmod_modules_order_for_compressed(const struct test *t)
{
	EXEC_DEPMOD();
	exit(EXIT_FAILURE);
}
DEFINE_TEST(depmod_modules_order_for_compressed,
	.description = "check if depmod let aliases in right order when using compressed modules",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = "test-depmod/modules-order-compressed",
	},
	.output = {
		.files = (const struct keyval[]) {
			{ "correct-modules.alias", "modules.alias" },
			{ },
		},
	});

#define MODULES_OUTDIR_LIB_MODULES_OUTPUT \
	MODULES_OUTDIR_ROOTFS "/outdir" MODULE_DIRECTORY "/" "4.4.4"
static noreturn int depmod_modules_outdir(const struct test *t)
{
	EXEC_DEPMOD("--outdir", "/outdir/");
	exit(EXIT_FAILURE);
}
DEFINE_TEST(depmod_modules_outdir,
	.description = "check if depmod honours the outdir option",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = "test-depmod/modules-outdir",
	},
	.output = {
		.files = (const struct keyval[]) {
			{ MODULES_OUTDIR_LIB_MODULES_OUTPUT "/modules.dep",
			  MODULES_OUTDIR_ROOTFS "/correct-modules.dep" },
			{ MODULES_OUTDIR_LIB_MODULES_OUTPUT "/modules.alias",
			  MODULES_OUTDIR_ROOTFS "/correct-modules.alias" },
			{ },
		},
	});

static noreturn int depmod_search_order_simple(const struct test *t)
{
	EXEC_DEPMOD();
	exit(EXIT_FAILURE);
}
DEFINE_TEST(depmod_search_order_simple,
	.description = "check if depmod honor search order in config",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = "test-depmod/search-order-simple"
	},
	.output = {
		.files = (const struct keyval[]) {
			{ "correct-modules.dep", "modules.dep" },
			{ },
		},
	});

#define ANOTHER_MODDIR "/foobar"
#define RELATIVE_MODDIR "foobar2"
#define MODULES_ANOTHER_MODDIR_ROOTFS "test-depmod/another-moddir"
static noreturn int depmod_another_moddir(const struct test *t)
{
	EXEC_DEPMOD("-m", ANOTHER_MODDIR);
	exit(EXIT_FAILURE);
}
static noreturn int depmod_another_moddir_relative(const struct test *t)
{
	EXEC_DEPMOD("-m", RELATIVE_MODDIR);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(depmod_another_moddir,
	.description = "check depmod -m flag",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = MODULES_ANOTHER_MODDIR_ROOTFS,
	},
	.output = {
		.files = (const struct keyval[]) {
			{ MODULES_ANOTHER_MODDIR_ROOTFS "/correct-modules.dep",
			  MODULES_ANOTHER_MODDIR_ROOTFS ANOTHER_MODDIR "/" "4.4.4" "/modules.dep" },
			{ },
		},
	});
DEFINE_TEST(depmod_another_moddir_relative,
	.description = "check depmod -m flag with relative dir",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = MODULES_ANOTHER_MODDIR_ROOTFS,
	},
	.output = {
		.files = (const struct keyval[]) {
			{ MODULES_ANOTHER_MODDIR_ROOTFS "/correct-modules.dep",
			  MODULES_ANOTHER_MODDIR_ROOTFS "/" RELATIVE_MODDIR "/" "4.4.4" "/modules.dep" },
			{ },
		},
	});

static noreturn int depmod_search_order_same_prefix(const struct test *t)
{
	EXEC_DEPMOD();
	exit(EXIT_FAILURE);
}
DEFINE_TEST(depmod_search_order_same_prefix,
	.description = "check if depmod honor search order in config with same prefix",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = "test-depmod/search-order-same-prefix",
	},
	.output = {
		.files = (const struct keyval[]) {
			{ "correct-modules.dep", "modules.dep" },
			{ },
		},
	});

#define DETECT_LOOP_ROOTFS "test-depmod/detect-loop"
static noreturn int depmod_detect_loop(const struct test *t)
{
	EXEC_DEPMOD();
	exit(EXIT_FAILURE);
}
DEFINE_TEST(depmod_detect_loop,
	.description = "check if depmod detects module loops correctly",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = DETECT_LOOP_ROOTFS,
	},
	.expected_fail = true,
	.output = {
		.err = DETECT_LOOP_ROOTFS "/correct.txt",
	});

#define SEARCH_ORDER_EXTERNAL_FIRST_ROOTFS "test-depmod/search-order-external-first"
#define SEARCH_ORDER_EXTERNAL_FIRST_LIB_MODULES \
	SEARCH_ORDER_EXTERNAL_FIRST_ROOTFS MODULE_DIRECTORY "/" "4.4.4"
static noreturn int depmod_search_order_external_first(const struct test *t)
{
	EXEC_DEPMOD();
	exit(EXIT_FAILURE);
}
DEFINE_TEST(depmod_search_order_external_first,
	.description = "check if depmod honor external keyword with higher priority",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = SEARCH_ORDER_EXTERNAL_FIRST_ROOTFS,
	},
	.output = {
		.files = (const struct keyval[]) {
			{ SEARCH_ORDER_EXTERNAL_FIRST_LIB_MODULES "/correct-modules.dep",
			  SEARCH_ORDER_EXTERNAL_FIRST_LIB_MODULES "/modules.dep" },
			{ },
		},
	});

#define SEARCH_ORDER_EXTERNAL_LAST_ROOTFS "test-depmod/search-order-external-last"
#define SEARCH_ORDER_EXTERNAL_LAST_LIB_MODULES \
	SEARCH_ORDER_EXTERNAL_LAST_ROOTFS MODULE_DIRECTORY "/" "4.4.4"
static noreturn int depmod_search_order_external_last(const struct test *t)
{
	EXEC_DEPMOD();
	exit(EXIT_FAILURE);
}
DEFINE_TEST(depmod_search_order_external_last,
	.description = "check if depmod honor external keyword with lower priority",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = SEARCH_ORDER_EXTERNAL_LAST_ROOTFS,
	},
	.output = {
		.files = (const struct keyval[]) {
			{ SEARCH_ORDER_EXTERNAL_LAST_LIB_MODULES "/correct-modules.dep",
			  SEARCH_ORDER_EXTERNAL_LAST_LIB_MODULES "/modules.dep" },
			{ },
		},
	});

#define SEARCH_ORDER_OVERRIDE_ROOTFS "test-depmod/search-order-override"
#define SEARCH_ORDER_OVERRIDE_LIB_MODULES \
	SEARCH_ORDER_OVERRIDE_ROOTFS MODULE_DIRECTORY "/" "4.4.4"
static noreturn int depmod_search_order_override(const struct test *t)
{
	EXEC_DEPMOD();
	exit(EXIT_FAILURE);
}
DEFINE_TEST(depmod_search_order_override,
	.description = "check if depmod honor override keyword",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = SEARCH_ORDER_OVERRIDE_ROOTFS,
	},
	.output = {
		.files = (const struct keyval[]) {
			{ SEARCH_ORDER_OVERRIDE_LIB_MODULES "/correct-modules.dep",
			  SEARCH_ORDER_OVERRIDE_LIB_MODULES "/modules.dep" },
			{ },
		},
	});

#define CHECK_WEAKDEP_LIB_MODULES CHECK_WEAKDEP_ROOTFS MODULE_DIRECTORY "/" "4.4.4"
static noreturn int depmod_check_weakdep(const struct test *t)
{
	EXEC_DEPMOD();
	exit(EXIT_FAILURE);
}
DEFINE_TEST(depmod_check_weakdep,
	.description = "check weakdep output",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = "test-depmod/check-weakdep",
	},
	.output = {
		.files = (const struct keyval[]) {
			{ "correct-modules.weakdep", "modules.weakdep" },
			{ },
		},
	});

TESTSUITE_MAIN();
