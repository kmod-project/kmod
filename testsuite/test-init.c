#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <libkmod.h>

#include "testsuite.h"

static int testsuite_init(const struct test *t)
{
	struct kmod_ctx *ctx;
	const char *null_config = NULL;

	ctx = kmod_new(NULL, &null_config);
	if (ctx == NULL)
		exit(EXIT_FAILURE);

	kmod_unref(ctx);

	exit(EXIT_SUCCESS);
}
static const struct test stestsuite_init = {
	.name = "testsuite_init",
	.description = "test if libkmod's init function work",
	.func = testsuite_init,
};

static const struct test *tests[] = {
	&stestsuite_init,
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
