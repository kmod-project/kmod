// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2012-2013  ProFUSION embedded systems
 */

#include <inttypes.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "testsuite.h"

#define MODULES_UNAME "4.4.4"
#define MODULES_ORDER_ROOTFS TESTSUITE_ROOTFS "test-depmod/modules-order-compressed"
#define MODULES_ORDER_LIB_MODULES MODULES_ORDER_ROOTFS MODULE_DIRECTORY "/" MODULES_UNAME
static int depmod_modules_order_for_compressed(void)
{
	return EXEC_TOOL(depmod);
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
			{ },
		},
	});

#define MODULES_OUTDIR_ROOTFS TESTSUITE_ROOTFS "test-depmod/modules-outdir"
#define MODULES_OUTDIR_LIB_MODULES_OUTPUT \
	MODULES_OUTDIR_ROOTFS "/outdir" MODULE_DIRECTORY "/" MODULES_UNAME
#define MODULES_OUTDIR_LIB_MODULES_INPUT \
	MODULES_OUTDIR_ROOTFS MODULE_DIRECTORY "/" MODULES_UNAME
static int depmod_modules_outdir(void)
{
	return EXEC_TOOL(depmod, "--outdir", "/outdir/");
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
			{ },
		},
	});

#define SEARCH_ORDER_SIMPLE_ROOTFS TESTSUITE_ROOTFS "test-depmod/search-order-simple"
#define SEARCH_ORDER_SIMPLE_LIB_MODULES \
	SEARCH_ORDER_SIMPLE_ROOTFS MODULE_DIRECTORY "/" MODULES_UNAME
static int depmod_search_order_simple(void)
{
	return EXEC_TOOL(depmod);
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
			{ },
		},
	});

#define ANOTHER_MODDIR "/foobar"
#define RELATIVE_MODDIR "foobar2"
#define MODULES_ANOTHER_MODDIR_ROOTFS TESTSUITE_ROOTFS "test-depmod/another-moddir"
static int depmod_another_moddir(void)
{
	return EXEC_TOOL(depmod, "-m", ANOTHER_MODDIR);
}
static int depmod_another_moddir_relative(void)
{
	return EXEC_TOOL(depmod, "-m", RELATIVE_MODDIR);
}
DEFINE_TEST(depmod_another_moddir,
	.description = "check depmod -m flag",
	.config = {
		[TC_UNAME_R] = MODULES_UNAME,
		[TC_ROOTFS] = MODULES_ANOTHER_MODDIR_ROOTFS,
	},
	.output = {
		.files = (const struct keyval[]) {
			{ MODULES_ANOTHER_MODDIR_ROOTFS "/correct-modules.dep",
			  MODULES_ANOTHER_MODDIR_ROOTFS ANOTHER_MODDIR "/" MODULES_UNAME "/modules.dep" },
			{ },
		},
	});
DEFINE_TEST(depmod_another_moddir_relative,
	.description = "check depmod -m flag with relative dir",
	.config = {
		[TC_UNAME_R] = MODULES_UNAME,
		[TC_ROOTFS] = MODULES_ANOTHER_MODDIR_ROOTFS,
	},
	.output = {
		.files = (const struct keyval[]) {
			{ MODULES_ANOTHER_MODDIR_ROOTFS "/correct-modules.dep",
			  MODULES_ANOTHER_MODDIR_ROOTFS "/" RELATIVE_MODDIR "/" MODULES_UNAME "/modules.dep" },
			{ },
		},
	});

#define SEARCH_ORDER_SAME_PREFIX_ROOTFS \
	TESTSUITE_ROOTFS "test-depmod/search-order-same-prefix"
#define SEARCH_ORDER_SAME_PREFIX_LIB_MODULES \
	SEARCH_ORDER_SAME_PREFIX_ROOTFS MODULE_DIRECTORY "/" MODULES_UNAME
static int depmod_search_order_same_prefix(void)
{
	return EXEC_TOOL(depmod);
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
			{ },
		},
	});

#define DETECT_LOOP_ROOTFS TESTSUITE_ROOTFS "test-depmod/detect-loop"
static int depmod_detect_loop(void)
{
	return EXEC_TOOL(depmod);
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

#define SEARCH_ORDER_EXTERNAL_FIRST_ROOTFS \
	TESTSUITE_ROOTFS "test-depmod/search-order-external-first"
#define SEARCH_ORDER_EXTERNAL_FIRST_LIB_MODULES \
	SEARCH_ORDER_EXTERNAL_FIRST_ROOTFS MODULE_DIRECTORY "/" MODULES_UNAME
static int depmod_search_order_external_first(void)
{
	return EXEC_TOOL(depmod);
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
			{ },
		},
	});

#define SEARCH_ORDER_EXTERNAL_LAST_ROOTFS \
	TESTSUITE_ROOTFS "test-depmod/search-order-external-last"
#define SEARCH_ORDER_EXTERNAL_LAST_LIB_MODULES \
	SEARCH_ORDER_EXTERNAL_LAST_ROOTFS MODULE_DIRECTORY "/" MODULES_UNAME
static int depmod_search_order_external_last(void)
{
	return EXEC_TOOL(depmod);
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
			{ },
		},
	});

#define SEARCH_ORDER_OVERRIDE_ROOTFS TESTSUITE_ROOTFS "test-depmod/search-order-override"
#define SEARCH_ORDER_OVERRIDE_LIB_MODULES \
	SEARCH_ORDER_OVERRIDE_ROOTFS MODULE_DIRECTORY "/" MODULES_UNAME
static int depmod_search_order_override(void)
{
	return EXEC_TOOL(depmod);
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
			{ },
		},
	});

#define CHECK_WEAKDEP_ROOTFS TESTSUITE_ROOTFS "test-depmod/check-weakdep"
#define CHECK_WEAKDEP_LIB_MODULES CHECK_WEAKDEP_ROOTFS MODULE_DIRECTORY "/" MODULES_UNAME
static int depmod_check_weakdep(void)
{
	return EXEC_TOOL(depmod);
}
DEFINE_TEST(depmod_check_weakdep,
	.description = "check weakdep output",
	.config = {
		[TC_UNAME_R] = MODULES_UNAME,
		[TC_ROOTFS] = CHECK_WEAKDEP_ROOTFS,
	},
	.output = {
		.files = (const struct keyval[]) {
			{ CHECK_WEAKDEP_LIB_MODULES "/correct-modules.weakdep",
			  CHECK_WEAKDEP_LIB_MODULES "/modules.weakdep" },
			{ },
		},
	});

TESTSUITE_MAIN();
