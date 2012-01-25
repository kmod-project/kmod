#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <libkmod.h>

#include "testsuite.h"


#define TEST_UNAME "4.0.20-kmod"
static int testsuite_uname(const struct test *t)
{
	struct utsname u;
	int err = uname(&u);

	if (err < 0)
		exit(EXIT_FAILURE);

	if (strcmp(u.release, TEST_UNAME) != 0) {
		char *ldpreload = getenv("LD_PRELOAD");
		ERR("u.release=%s should be %s\n", u.release, TEST_UNAME);
		ERR("LD_PRELOAD=%s\n", ldpreload);
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}
static const struct test stestsuite_uname = {
	.name = "testsuite_uname",
	.description = "test if trap to uname() works",
	.func = testsuite_uname,
	.config = {
		[TC_UNAME_R] = TEST_UNAME,
	},
	.need_spawn = true,
};

static const struct test *tests[] = {
	&stestsuite_uname,
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
