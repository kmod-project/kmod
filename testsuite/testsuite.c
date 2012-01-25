#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/wait.h>

#include "testsuite.h"

static const char *progname;
static int oneshot = 0;
static const char options_short[] = "lhn";
static const struct option options[] = {
	{ "list", no_argument, 0, 'l' },
	{ "help", no_argument, 0, 'h' },
	{ NULL, 0, 0, 0 }
};

static void help(void)
{
	const struct option *itr;
	const char *itr_short;

	printf("Usage:\n"
	       "\t%s [options] <test>\n"
	       "Options:\n", basename(progname));

	for (itr = options, itr_short = options_short;
				itr->name != NULL; itr++, itr_short++)
		printf("\t-%c, --%s\n", *itr_short, itr->name);
}

static void test_list(const struct test *tests[])
{
	size_t i;

	printf("Available tests:\n");
	for (i = 0; tests[i] != NULL; i++)
		printf("\t%s, %s\n", tests[i]->name, tests[i]->description);
}

int test_init(int argc, char *const argv[], const struct test *tests[])
{
	progname = argv[0];

	for (;;) {
		int c, idx = 0;
		c = getopt_long(argc, argv, options_short, options, &idx);
		if (c == -1)
			break;
		switch (c) {
		case 'l':
			test_list(tests);
			return 0;
		case 'h':
			help();
			return 0;
		case 'n':
			oneshot = 1;
			break;
		case '?':
			return -1;
		default:
			ERR("unexpected getopt_long() value %c\n", c);
			return -1;
		}
	}

	return optind;
}

const struct test *test_find(const struct test *tests[], const char *name)
{
	size_t i;

	for (i = 0; tests[i] != NULL; i++) {
		if (strcmp(tests[i]->name, name) == 0)
			return tests[i];
	}

	return NULL;
}

static int test_spawn_test(const struct test *t)
{
	const char *const args[] = { progname, "-n", t->name, NULL };

	execv(progname, (char *const *) args);

	ERR("failed to spawn %s for %s: %m\n", progname, t->name);
	return EXIT_FAILURE;
}

static int test_run_spawned(const struct test *t)
{
	int err = t->func(t);
	exit(err);

	return EXIT_FAILURE;
}

int test_spawn_prog(const char *prog, const char *args[])
{
	execv(prog, (char *const *) args);

	ERR("failed to spawn %s ", prog);
	return EXIT_FAILURE;
}

{
}

int test_run(const struct test *t)
{
	int err;
	pid_t pid;

	if (t->need_spawn && oneshot)
		test_run_spawned(t);

	LOG("running %s, in forked context\n", t->name);

	pid = fork();
	if (pid < 0) {
		ERR("could not fork(): %m\n");
		LOG("FAILED: %s\n", t->name);
		return EXIT_FAILURE;
	}

	if (pid > 0) {
		do {
			pid = wait(&err);
			if (pid == -1) {
				ERR("error waitpid(): %m\n");
				return EXIT_FAILURE;
			}
		} while (!WIFEXITED(err) && !WIFSIGNALED(err));

		if (err != 0)
			ERR("error while running %s\n", t->name);

		LOG("%s: %s\n", err == 0 ? "PASSED" : "FAILED", t->name);
		return err;
	}

        /* kill child if parent dies */
        prctl(PR_SET_PDEATHSIG, SIGTERM);

	if (t->need_spawn)
		return test_spawn_test(t);
	else
		return test_run_spawned(t);
}
