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

#define MODULES_UNAME "4.4.4"
#define MODULES_ORDER_ROOTFS TESTSUITE_ROOTFS "test-depmod/modules-order-compressed"
#define MODULES_ORDER_LIB_MODULES MODULES_ORDER_ROOTFS MODULE_DIRECTORY "/" MODULES_UNAME
static noreturn int depmod_modules_order_for_compressed(const struct test *t)
{
	EXEC_DEPMOD();
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
			{ },
		},
	});

#define MODULES_OUTDIR_ROOTFS TESTSUITE_ROOTFS "test-depmod/modules-outdir"
#define MODULES_OUTDIR_LIB_MODULES_OUTPUT \
	MODULES_OUTDIR_ROOTFS "/outdir" MODULE_DIRECTORY "/" MODULES_UNAME
#define MODULES_OUTDIR_LIB_MODULES_INPUT \
	MODULES_OUTDIR_ROOTFS MODULE_DIRECTORY "/" MODULES_UNAME
static noreturn int depmod_modules_outdir(const struct test *t)
{
	EXEC_DEPMOD("--outdir", "/outdir/");
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
			{ },
		},
	});

#define SEARCH_ORDER_SIMPLE_ROOTFS TESTSUITE_ROOTFS "test-depmod/search-order-simple"
#define SEARCH_ORDER_SIMPLE_LIB_MODULES \
	SEARCH_ORDER_SIMPLE_ROOTFS MODULE_DIRECTORY "/" MODULES_UNAME
static noreturn int depmod_search_order_simple(const struct test *t)
{
	EXEC_DEPMOD();
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
			{ },
		},
	});

#define ANOTHER_MODDIR "/foobar"
#define RELATIVE_MODDIR "foobar2"
#define MODULES_ANOTHER_MODDIR_ROOTFS TESTSUITE_ROOTFS "test-depmod/another-moddir"
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
static noreturn int depmod_search_order_same_prefix(const struct test *t)
{
	EXEC_DEPMOD();
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
			{ },
		},
	});

#define DETECT_LOOP_ROOTFS TESTSUITE_ROOTFS "test-depmod/detect-loop"
static noreturn int depmod_detect_loop(const struct test *t)
{
	EXEC_DEPMOD();
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

#define SEARCH_ORDER_EXTERNAL_FIRST_ROOTFS \
	TESTSUITE_ROOTFS "test-depmod/search-order-external-first"
#define SEARCH_ORDER_EXTERNAL_FIRST_LIB_MODULES \
	SEARCH_ORDER_EXTERNAL_FIRST_ROOTFS MODULE_DIRECTORY "/" MODULES_UNAME
static noreturn int depmod_search_order_external_first(const struct test *t)
{
	EXEC_DEPMOD();
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
			{ },
		},
	});

#define SEARCH_ORDER_EXTERNAL_LAST_ROOTFS \
	TESTSUITE_ROOTFS "test-depmod/search-order-external-last"
#define SEARCH_ORDER_EXTERNAL_LAST_LIB_MODULES \
	SEARCH_ORDER_EXTERNAL_LAST_ROOTFS MODULE_DIRECTORY "/" MODULES_UNAME
static noreturn int depmod_search_order_external_last(const struct test *t)
{
	EXEC_DEPMOD();
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
			{ },
		},
	});

#define SEARCH_ORDER_OVERRIDE_ROOTFS TESTSUITE_ROOTFS "test-depmod/search-order-override"
#define SEARCH_ORDER_OVERRIDE_LIB_MODULES \
	SEARCH_ORDER_OVERRIDE_ROOTFS MODULE_DIRECTORY "/" MODULES_UNAME
static noreturn int depmod_search_order_override(const struct test *t)
{
	EXEC_DEPMOD();
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
			{ },
		},
	});

#define CHECK_WEAKDEP_ROOTFS TESTSUITE_ROOTFS "test-depmod/check-weakdep"
#define CHECK_WEAKDEP_LIB_MODULES CHECK_WEAKDEP_ROOTFS MODULE_DIRECTORY "/" MODULES_UNAME
static noreturn int depmod_check_weakdep(const struct test *t)
{
	EXEC_DEPMOD();
	exit(EXIT_FAILURE);
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

#define TEST_DEPENDENCIES_ROOTFS TESTSUITE_ROOTFS "test-depmod/test-dependencies"
static noreturn int depmod_test_dependencies(const struct test *t)
{
	const char *progname = TOOLS_DIR "/depmod";
	const char *const args[] = {
		progname,
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(depmod_test_dependencies,
#if defined(KMOD_SYSCONFDIR_NOT_ETC)
        .skip = true,
#endif
	.description = "check if depmod generates correct dependencies",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TEST_DEPENDENCIES_ROOTFS,
	},
	.output = {
		.files = (const struct keyval[]) {
			{ TEST_DEPENDENCIES_ROOTFS "/lib/modules/4.4.4/correct-modules.dep",
			  TEST_DEPENDENCIES_ROOTFS "/lib/modules/4.4.4/modules.dep" },
			{ TEST_DEPENDENCIES_ROOTFS "/lib/modules/4.4.4/correct-modules.dep.bin",
			  TEST_DEPENDENCIES_ROOTFS "/lib/modules/4.4.4/modules.dep.bin" },
			{ TEST_DEPENDENCIES_ROOTFS "/lib/modules/4.4.4/correct-modules.symbols",
			  TEST_DEPENDENCIES_ROOTFS "/lib/modules/4.4.4/modules.symbols" },
			{ TEST_DEPENDENCIES_ROOTFS "/lib/modules/4.4.4/correct-modules.symbols.bin",
			  TEST_DEPENDENCIES_ROOTFS "/lib/modules/4.4.4/modules.symbols.bin" },
			{ }
		},
	});

#define TEST_DEPENDENCIES_1_ROOTFS TESTSUITE_ROOTFS "test-depmod/test-dependencies-1"
static noreturn int depmod_test_dependencies_1(const struct test *t)
{
	const char *progname = TOOLS_DIR "/depmod";
	const char *const args[] = {
		progname,
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(depmod_test_dependencies_1,
#if defined(KMOD_SYSCONFDIR_NOT_ETC)
        .skip = true,
#endif
	.description = "check dependency generation, case 1: mod_bar",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TEST_DEPENDENCIES_1_ROOTFS,
	},
	.output = {
		.regex = true,
		.files = (const struct keyval[]) {
			{ TEST_DEPENDENCIES_1_ROOTFS "/lib/modules/4.4.4/correct-modules.dep",
			  TEST_DEPENDENCIES_1_ROOTFS "/lib/modules/4.4.4/modules.dep" },
			{ TEST_DEPENDENCIES_1_ROOTFS "/lib/modules/4.4.4/correct-modules.dep.bin",
			  TEST_DEPENDENCIES_1_ROOTFS "/lib/modules/4.4.4/modules.dep.bin" },
			{ TEST_DEPENDENCIES_1_ROOTFS "/lib/modules/4.4.4/correct-modules.symbols",
			  TEST_DEPENDENCIES_1_ROOTFS "/lib/modules/4.4.4/modules.symbols" },
			{ TEST_DEPENDENCIES_1_ROOTFS "/lib/modules/4.4.4/correct-modules.symbols.bin",
			  TEST_DEPENDENCIES_1_ROOTFS "/lib/modules/4.4.4/modules.symbols.bin" },
			{ }
		},
	});

#define TEST_DEPENDENCIES_2_ROOTFS TESTSUITE_ROOTFS "test-depmod/test-dependencies-2"
static noreturn int depmod_test_dependencies_2(const struct test *t)
{
	const char *progname = TOOLS_DIR "/depmod";
	const char *const args[] = {
		progname,
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(depmod_test_dependencies_2,
#if defined(KMOD_SYSCONFDIR_NOT_ETC)
        .skip = true,
#endif
	.description = "check dependency generation, case 2: mod_bah",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TEST_DEPENDENCIES_2_ROOTFS,
	},
	.output = {
		.regex = true,
		.files = (const struct keyval[]) {
			{ TEST_DEPENDENCIES_2_ROOTFS "/lib/modules/4.4.4/correct-modules.dep",
			  TEST_DEPENDENCIES_2_ROOTFS "/lib/modules/4.4.4/modules.dep" },
			{ TEST_DEPENDENCIES_2_ROOTFS "/lib/modules/4.4.4/correct-modules.dep.bin",
			  TEST_DEPENDENCIES_2_ROOTFS "/lib/modules/4.4.4/modules.dep.bin" },
			{ TEST_DEPENDENCIES_2_ROOTFS "/lib/modules/4.4.4/correct-modules.symbols",
			  TEST_DEPENDENCIES_2_ROOTFS "/lib/modules/4.4.4/modules.symbols" },
			{ TEST_DEPENDENCIES_2_ROOTFS "/lib/modules/4.4.4/correct-modules.symbols.bin",
			  TEST_DEPENDENCIES_2_ROOTFS "/lib/modules/4.4.4/modules.symbols.bin" },
			{ }
		},
	});

#define TEST_BIG_01_ROOTFS TESTSUITE_ROOTFS "test-depmod/big-01"
static noreturn int depmod_test_big_01(const struct test *t)
{
	const char *progname = TOOLS_DIR "/depmod";
	const char *const args[] = {
		progname, "-e", "-E", TEST_BIG_01_ROOTFS "/lib/modules/5.3.18/symvers",
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(depmod_test_big_01,
#if defined(KMOD_SYSCONFDIR_NOT_ETC)
        .skip = true,
#endif
	.description = "check generating dependencies for a larger set of modules",
	.config = {
		[TC_UNAME_R] = "5.3.18",
		[TC_ROOTFS] = TEST_BIG_01_ROOTFS,
	},
	.output = {
		.regex = true,
		.files = (const struct keyval[]) {
			{ TEST_BIG_01_ROOTFS "/lib/modules/5.3.18/correct-modules.dep",
			  TEST_BIG_01_ROOTFS "/lib/modules/5.3.18/modules.dep" },
			{ TEST_BIG_01_ROOTFS "/lib/modules/5.3.18/correct-modules.dep.bin",
			  TEST_BIG_01_ROOTFS "/lib/modules/5.3.18/modules.dep.bin" },
			{ TEST_BIG_01_ROOTFS "/lib/modules/5.3.18/correct-modules.symbols",
			  TEST_BIG_01_ROOTFS "/lib/modules/5.3.18/modules.symbols" },
			{ TEST_BIG_01_ROOTFS "/lib/modules/5.3.18/correct-modules.symbols.bin",
			  TEST_BIG_01_ROOTFS "/lib/modules/5.3.18/modules.symbols.bin" },
			{ }
		},
	});

#define TEST_BIG_01_INCREMENTAL_ROOTFS TESTSUITE_ROOTFS "test-depmod/big-01-incremental"
static noreturn int depmod_test_big_01_incremental(const struct test *t)
{
	const char *progname = TOOLS_DIR "/depmod";
	const char *const args[] = {
		progname, "-e",
		"-E",	  TEST_BIG_01_INCREMENTAL_ROOTFS "/lib/modules/5.3.18/symvers",
		"-I",	  "-a",
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(depmod_test_big_01_incremental,
#if defined(KMOD_SYSCONFDIR_NOT_ETC)
        .skip = true,
#endif
	.description = "check generating incremental dependencies the big-01 module set",
	.config = {
		[TC_UNAME_R] = "5.3.18",
		[TC_ROOTFS] = TEST_BIG_01_INCREMENTAL_ROOTFS,
	},
	.output = {
		.regex = true,
		.files = (const struct keyval[]) {
			{ TEST_BIG_01_INCREMENTAL_ROOTFS "/lib/modules/5.3.18/correct-modules.dep",
			  TEST_BIG_01_INCREMENTAL_ROOTFS "/lib/modules/5.3.18/modules.dep" },
			{ TEST_BIG_01_INCREMENTAL_ROOTFS "/lib/modules/5.3.18/correct-modules.dep.bin",
			  TEST_BIG_01_INCREMENTAL_ROOTFS "/lib/modules/5.3.18/modules.dep.bin" },
			{ TEST_BIG_01_INCREMENTAL_ROOTFS "/lib/modules/5.3.18/correct-modules.symbols",
			  TEST_BIG_01_INCREMENTAL_ROOTFS "/lib/modules/5.3.18/modules.symbols" },
			{ TEST_BIG_01_INCREMENTAL_ROOTFS "/lib/modules/5.3.18/correct-modules.symbols.bin",
			  TEST_BIG_01_INCREMENTAL_ROOTFS "/lib/modules/5.3.18/modules.symbols.bin" },
			{ }
		},
	});

#define TEST_BIG_01_DELETE_ROOTFS TESTSUITE_ROOTFS "test-depmod/big-01-delete"
static noreturn int depmod_test_big_01_delete(const struct test *t)
{
	const char *progname = TOOLS_DIR "/depmod";
	const char *const args[] = {
		progname, "-e",
		"-E",	  TEST_BIG_01_DELETE_ROOTFS "/lib/modules/5.3.18/symvers",
		"-I",	  "-a",
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(depmod_test_big_01_delete,
#if defined(KMOD_SYSCONFDIR_NOT_ETC)
        .skip = true,
#endif
	.description = "check depmod -I with module deletion from the big-01 module set",
	.config = {
		[TC_UNAME_R] = "5.3.18",
		[TC_ROOTFS] = TEST_BIG_01_DELETE_ROOTFS,
	},
	.output = {
		.regex = true,
		.files = (const struct keyval[]) {
			{ TEST_BIG_01_DELETE_ROOTFS "/lib/modules/5.3.18/correct-modules.dep",
			  TEST_BIG_01_DELETE_ROOTFS "/lib/modules/5.3.18/modules.dep" },
			{ TEST_BIG_01_DELETE_ROOTFS "/lib/modules/5.3.18/correct-modules.dep.bin",
			  TEST_BIG_01_DELETE_ROOTFS "/lib/modules/5.3.18/modules.dep.bin" },
			{ TEST_BIG_01_DELETE_ROOTFS "/lib/modules/5.3.18/correct-modules.symbols",
			  TEST_BIG_01_DELETE_ROOTFS "/lib/modules/5.3.18/modules.symbols" },
			{ TEST_BIG_01_DELETE_ROOTFS "/lib/modules/5.3.18/correct-modules.symbols.bin",
			  TEST_BIG_01_DELETE_ROOTFS "/lib/modules/5.3.18/modules.symbols.bin" },
			{ }
		},
	});

#define TEST_BIG_01_REPLACE_ROOTFS TESTSUITE_ROOTFS "test-depmod/big-01-replace"
static noreturn int depmod_test_big_01_replace(const struct test *t)
{
	const char *progname = TOOLS_DIR "/depmod";
	const char *const args[] = {
		progname,
		"-e",
		"-E",
		TEST_BIG_01_REPLACE_ROOTFS "/lib/modules/5.3.18/symvers",
		"-I",
		"5.3.18",
		"kernel/drivers/scsi/qla2xxx/qla2xxx.ko",
		"kernel/drivers/scsi/qla2xxx/tcm_qla2xxx.ko",
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(depmod_test_big_01_replace,
#if defined(KMOD_SYSCONFDIR_NOT_ETC)
        .skip = true,
#endif
	.description = "check depmod -I with module deletion from the big-01 module set",
	.config = {
		[TC_UNAME_R] = "5.3.18",
		[TC_ROOTFS] = TEST_BIG_01_REPLACE_ROOTFS,
	},
	.output = {
		.regex = true,
		.files = (const struct keyval[]) {
			{ TEST_BIG_01_REPLACE_ROOTFS "/lib/modules/5.3.18/correct-modules.dep",
			  TEST_BIG_01_REPLACE_ROOTFS "/lib/modules/5.3.18/modules.dep" },
			{ TEST_BIG_01_REPLACE_ROOTFS "/lib/modules/5.3.18/correct-modules.dep.bin",
			  TEST_BIG_01_REPLACE_ROOTFS "/lib/modules/5.3.18/modules.dep.bin" },
			{ TEST_BIG_01_REPLACE_ROOTFS "/lib/modules/5.3.18/correct-modules.symbols",
			  TEST_BIG_01_REPLACE_ROOTFS "/lib/modules/5.3.18/modules.symbols" },
			{ TEST_BIG_01_REPLACE_ROOTFS "/lib/modules/5.3.18/correct-modules.symbols.bin",
			  TEST_BIG_01_REPLACE_ROOTFS "/lib/modules/5.3.18/modules.symbols.bin" },
			{ }
		},
	});

TESTSUITE_MAIN();
