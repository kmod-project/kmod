/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2012-2013  ProFUSION embedded systems
 */

#pragma once

#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

#include <shared/macro.h>

struct test;
typedef int (*testfunc)(void);

enum test_config {
	/*
	 * Where's the roots dir for this test. It will LD_PRELOAD path.so in
	 * order to trap calls to functions using paths.
	 */
	TC_ROOTFS = 0,

	/*
	 * What's the desired string to be returned by `uname -r`. It will
	 * trap calls to uname(3P) by LD_PRELOAD'ing uname.so and then filling
	 * in the information in u.release.
	 */
	TC_UNAME_R,

	/*
	 * Fake calls to init_module(2), returning return-code and setting
	 * errno to err-code. Set this variable with the following format:
	 *
	 *        modname:return-code:err-code
	 *
	 * When this variable is used, all calls to init_module() are trapped
	 * and by default the return code is 0. In other words, they fake
	 * "success" for all modules, except the ones in the list above, for
	 * which the return codes are used.
	 */
	TC_INIT_MODULE_RETCODES,

	/*
	 * Fake calls to delete_module(2), returning return-code and setting
	 * errno to err-code. Set this variable with the following format:
	 *
	 *        modname:return-code:err-code
	 *
	 * When this variable is used, all calls to init_module() are trapped
	 * and by default the return code is 0. In other words, they fake
	 * "success" for all modules, except the ones in the list above, for
	 * which the return codes are used.
	 */
	TC_DELETE_MODULE_RETCODES,

	_TC_LAST,
};

#define S_TC_ROOTFS "TESTSUITE_ROOTFS"
#define S_TC_UNAME_R "TESTSUITE_UNAME_R"
#define S_TC_INIT_MODULE_RETCODES "TESTSUITE_INIT_MODULE_RETCODES"
#define S_TC_DELETE_MODULE_RETCODES "TESTSUITE_DELETE_MODULE_RETCODES"

struct keyval {
	const char *key;
	const char *val;
};

struct test {
	const char *name;
	const char *description;
	struct {
		/* File with correct stdout */
		const char *out;
		/* File with correct stderr */
		const char *err;

		/*
		 * whether to treat the correct files as regex to the real
		 * output
		 */
		bool regex;

		/*
		 * Vector with pair of files
		 * key = correct file
		 * val = file to check
		 */
		const struct keyval *files;
	} output;
	/* comma-separated list of loaded modules at the end of the test */
	const char *modules_loaded;
	const char *modules_not_loaded;
	testfunc func;
	const char *config[_TC_LAST];
	const char *path;
	const struct keyval *env_vars;
	bool expected_fail;
	/* allow to skip tests that don't meet compile-time dependencies */
	bool skip;
};

int test_init(const struct test *start, const struct test *stop, int argc,
	      char *const argv[]);
const struct test *test_find(const struct test *start, const struct test *stop,
			     const char *name);
int test_spawn_prog(const char *prog, const char *const args[]);
int test_run(const struct test *t);

#define EXEC_TOOL(tool, ...)                 \
	test_spawn_prog(TOOLS_DIR "/" #tool, \
			(const char *[]){ TOOLS_DIR "/" #tool, ##__VA_ARGS__, NULL })

#define TS_EXPORT __attribute__((visibility("default")))

#define _LOG(prefix, fmt, ...) printf("TESTSUITE: " prefix fmt, ##__VA_ARGS__)
#define LOG(fmt, ...) _LOG("", fmt, ##__VA_ARGS__)
#define WARN(fmt, ...) _LOG("WARN: ", fmt, ##__VA_ARGS__)
#define ERR(fmt, ...) _LOG("ERR: ", fmt, ##__VA_ARGS__)

#define assert_return(expr, r)                                                  \
	do {                                                                    \
		if ((!(expr))) {                                                \
			ERR("Failed assertion: " #expr " %s:%d %s\n", __FILE__, \
			    __LINE__, __PRETTY_FUNCTION__);                     \
			return (r);                                             \
		}                                                               \
	} while (false)

/* Test definitions */
#define DEFINE_TEST_WITH_FUNC(_name, _func, ...)                                      \
	_used_ _retain_ _section_("kmod_tests") _alignedptr_ static const struct test \
	UNIQ(s##_name) = { .name = #_name, .func = _func, ##__VA_ARGS__ }

#define DEFINE_TEST(_name, ...) DEFINE_TEST_WITH_FUNC(_name, _name, __VA_ARGS__)

#define TESTSUITE_MAIN()                                                                 \
	extern struct test __start_kmod_tests[] __attribute__((visibility("hidden")));   \
	extern struct test __stop_kmod_tests[] __attribute__((visibility("hidden")));    \
	int main(int argc, char *argv[])                                                 \
	{                                                                                \
		const struct test *t;                                                    \
		int arg, ret = EXIT_SUCCESS;                                             \
                                                                                         \
		arg = test_init(__start_kmod_tests, __stop_kmod_tests, argc, argv);      \
		/* Invalid arguments */                                                  \
		if (arg < 0)                                                             \
			return EXIT_FAILURE;                                             \
		/* Print and exit options - list, help */                                \
		if (arg == 0)                                                            \
			return 0;                                                        \
                                                                                         \
		if (arg < argc) {                                                        \
			t = test_find(__start_kmod_tests, __stop_kmod_tests, argv[arg]); \
			if (t == NULL) {                                                 \
				fprintf(stderr, "could not find test %s\n", argv[arg]);  \
				return EXIT_FAILURE;                                     \
			}                                                                \
                                                                                         \
			return test_run(t);                                              \
		}                                                                        \
                                                                                         \
		for (t = __start_kmod_tests; t < __stop_kmod_tests; t++) {               \
			if (test_run(t) != 0)                                            \
				ret = EXIT_FAILURE;                                      \
		}                                                                        \
                                                                                         \
		return ret;                                                              \
	}
