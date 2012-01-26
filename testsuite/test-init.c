#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <libkmod.h>

#include "testsuite.h"

static int test_initlib(const struct test *t)
{
	struct kmod_ctx *ctx;
	const char *null_config = NULL;

	ctx = kmod_new(NULL, &null_config);
	if (ctx == NULL)
		exit(EXIT_FAILURE);

	kmod_unref(ctx);

	exit(EXIT_SUCCESS);
}
static const struct test stest_initlib = {
	.name = "test_initlib",
	.description = "test if libkmod's init function work",
	.func = test_initlib,
};

static int test_insert(const struct test *t)
{
	struct kmod_ctx *ctx;
	struct kmod_module *mod;
	const char *null_config = NULL;
	int err;

	ctx = kmod_new(NULL, &null_config);
	if (ctx == NULL)
		exit(EXIT_FAILURE);

	err = kmod_module_new_from_path(ctx, "/ext4-x86_64.ko", &mod);
	if (err != 0) {
		ERR("could not create module from path: %m\n");
		exit(EXIT_FAILURE);
	}

	err = kmod_module_insert_module(mod, 0, NULL);
	if (err != 0) {
		ERR("could not insert module: %m\n");
		exit(EXIT_FAILURE);
	}
	kmod_unref(ctx);

	exit(EXIT_SUCCESS);
}
static const struct test stest_insert = {
	.name = "test_insert",
	.description = "test if libkmod's insert_module returns ok",
	.func = test_insert,
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modinfo/",
		[TC_INIT_MODULE_RETCODES] = "bla:1:20",
	},
	.need_spawn = true,
};

static const struct test *tests[] = {
	&stest_initlib,
	&stest_insert,
	NULL,
};

int main(int argc, char *argv[])
{
	const struct test *t;
	int arg;
	size_t i;

	arg = test_init(argc, argv, tests);
	if (arg == 0)
		return 0;

	if (arg < argc) {
		t = test_find(tests, argv[arg]);
		if (t == NULL) {
			fprintf(stderr, "could not find test %s\n", argv[arg]);
			exit(EXIT_FAILURE);
		}

		return test_run(t);
	}

	for (i = 0; tests[i] != NULL; i++) {
		if (test_run(tests[i]) != 0)
			exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}
