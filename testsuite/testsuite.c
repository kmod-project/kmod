/*
 * Copyright (C) 2012  ProFUSION embedded systems
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/prctl.h>
#include <sys/wait.h>

#include "testsuite.h"

static const char *ANSI_HIGHLIGHT_GREEN_ON = "\x1B[1;32m";
static const char *ANSI_HIGHLIGHT_RED_ON =  "\x1B[1;31m";
static const char *ANSI_HIGHLIGHT_OFF = "\x1B[0m";

static const char *progname;
static int oneshot = 0;
static const char options_short[] = "lhn";
static const struct option options[] = {
	{ "list", no_argument, 0, 'l' },
	{ "help", no_argument, 0, 'h' },
	{ NULL, 0, 0, 0 }
};

#define OVERRIDE_LIBDIR ABS_TOP_BUILDDIR "/testsuite/.libs/"

struct _env_config {
	const char *key;
	const char *ldpreload;
} env_config[_TC_LAST] = {
	[TC_UNAME_R] = { S_TC_UNAME_R, OVERRIDE_LIBDIR  "uname.so" },
	[TC_ROOTFS] = { S_TC_ROOTFS, OVERRIDE_LIBDIR "path.so" },
	[TC_INIT_MODULE_RETCODES] = { S_TC_INIT_MODULE_RETCODES, OVERRIDE_LIBDIR "init_module.so" },
	[TC_DELETE_MODULE_RETCODES] = { S_TC_DELETE_MODULE_RETCODES, OVERRIDE_LIBDIR "delete_module.so" },
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

	if (isatty(STDOUT_FILENO) == 0) {
		ANSI_HIGHLIGHT_OFF = "";
		ANSI_HIGHLIGHT_RED_ON = "";
		ANSI_HIGHLIGHT_GREEN_ON = "";
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

int test_spawn_prog(const char *prog, const char *const args[])
{
	execv(prog, (char *const *) args);

	ERR("failed to spawn %s\n", prog);
	ERR("did you forget to build tools?\n");
	return EXIT_FAILURE;
}

static void test_export_environ(const struct test *t)
{
	char *preload = NULL;
	size_t preloadlen = 0;
	size_t i;

	unsetenv("LD_PRELOAD");

	for (i = 0; i < _TC_LAST; i++) {
		const char *ldpreload;
		size_t ldpreloadlen;
		char *tmp;

		if (t->config[i] == NULL)
			continue;

		setenv(env_config[i].key, t->config[i], 1);

		ldpreload = env_config[i].ldpreload;
		ldpreloadlen = strlen(ldpreload);
		tmp = realloc(preload, preloadlen + 2 + ldpreloadlen);
		if (tmp == NULL) {
			ERR("oom: test_export_environ()\n");
			return;
		}
		preload = tmp;

		if (preloadlen > 0)
			preload[preloadlen++] = ' ';
		memcpy(preload + preloadlen, ldpreload, ldpreloadlen);
		preloadlen += ldpreloadlen;
		preload[preloadlen] = '\0';
	}

	if (preload != NULL)
		setenv("LD_PRELOAD", preload, 1);

	free(preload);
}

static inline int test_run_child(const struct test *t, int fdout[2],
								int fderr[2])
{
	/* kill child if parent dies */
	prctl(PR_SET_PDEATHSIG, SIGTERM);

	test_export_environ(t);

	/* Close read-fds and redirect std{out,err} to the write-fds */
	if (t->output.stdout != NULL) {
		close(fdout[0]);
		if (dup2(fdout[1], STDOUT_FILENO) < 0) {
			ERR("could not redirect stdout to pipe: %m\n");
			exit(EXIT_FAILURE);
		}
	}

	if (t->output.stderr != NULL) {
		close(fderr[0]);
		if (dup2(fderr[1], STDERR_FILENO) < 0) {
			ERR("could not redirect stdout to pipe: %m\n");
			exit(EXIT_FAILURE);
		}
	}

	if (t->need_spawn)
		return test_spawn_test(t);
	else
		return test_run_spawned(t);
}

static inline bool test_run_parent_check_outputs(const struct test *t,
							int fdout, int fderr)
{
	struct epoll_event ep_outpipe, ep_errpipe;
	int err, fd_ep, fd_matchout = -1, fd_matcherr = -1;

	if (t->output.stdout == NULL && t->output.stderr == NULL)
		return true;

	fd_ep = epoll_create1(EPOLL_CLOEXEC);
	if (fd_ep < 0) {
		ERR("could not create epoll fd: %m\n");
		return false;
	}

	if (t->output.stdout != NULL) {
		fd_matchout = open(t->output.stdout, O_RDONLY);
		if (fd_matchout < 0) {
			err = -errno;
			ERR("could not open %s for read: %m\n",
							t->output.stdout);
			goto out;
		}
		memset(&ep_outpipe, 0, sizeof(struct epoll_event));
		ep_outpipe.events = EPOLLIN;
		ep_outpipe.data.ptr = &fdout;
		if (epoll_ctl(fd_ep, EPOLL_CTL_ADD, fdout, &ep_outpipe) < 0) {
			err = -errno;
			ERR("could not add fd to epoll: %m\n");
			goto out;
		}
	} else
		fdout = -1;

	if (t->output.stderr != NULL) {
		fd_matcherr = open(t->output.stderr, O_RDONLY);
		if (fd_matcherr < 0) {
			err = -errno;
			ERR("could not open %s for read: %m\n",
					t->output.stderr);
			goto out;

		}
		memset(&ep_errpipe, 0, sizeof(struct epoll_event));
		ep_errpipe.events = EPOLLIN;
		ep_errpipe.data.ptr = &fderr;
		if (epoll_ctl(fd_ep, EPOLL_CTL_ADD, fderr, &ep_errpipe) < 0) {
			err = -errno;
			ERR("could not add fd to epoll: %m\n");
			goto out;
		}
	} else
		fderr = -1;

	for (err = 0; fdout >= 0 || fderr >= 0;) {
		int fdcount, i;
		struct epoll_event ev[4];

		fdcount = epoll_wait(fd_ep, ev, 4, -1);
		if (fdcount < 0) {
			if (errno == EINTR)
				continue;
			err = -errno;
			ERR("could not poll: %m\n");
			goto out;
		}

		for (i = 0;  i < fdcount; i++) {
			int *fd = ev[i].data.ptr;

			if (ev[i].events & EPOLLIN) {
				ssize_t r, done = 0;
				char buf[4096];
				char bufmatch[4096];
				int fd_match;

				/*
				 * compare the output from child with the one
				 * saved as correct
				 */

				r = read(*fd, buf, sizeof(buf) - 1);
				if (r <= 0)
					continue;

				if (*fd == fdout)
					fd_match = fd_matchout;
				else
					fd_match = fd_matcherr;

				for (;;) {
					int rmatch = read(fd_match,
						bufmatch + done, r - done);
					if (rmatch == 0)
						break;

					if (rmatch < 0) {
						if (errno == EINTR)
							continue;
						err = -errno;
						ERR("could not read match fd %d\n",
								fd_match);
						goto out;
					}

					done += rmatch;
				}

				buf[r] = '\0';
				bufmatch[r] = '\0';
				if (strcmp(buf, bufmatch) != 0) {
					ERR("Outputs do not match on %s:\n",
						fd_match == fd_matchout ? "stdout" : "stderr");
					ERR("correct:\n%s\n", bufmatch);
					ERR("wrong:\n%s\n", buf);
					err = -1;
					goto out;
				}
			} else if (ev[i].events & EPOLLHUP) {
				if (epoll_ctl(fd_ep, EPOLL_CTL_DEL,
							*fd, NULL) < 0) {
					ERR("could not remove fd %d from epoll: %m\n",
									*fd);
				}
				*fd = -1;
			}
		}

	}
out:
	if (fd_matchout >= 0)
		close(fd_matchout);
	if (fd_matcherr >= 0)
		close(fd_matcherr);
	if (fd_ep >= 0)
		close(fd_ep);
	return err == 0;
}

static inline int test_run_parent(const struct test *t, int fdout[2],
								int fderr[2])
{
	pid_t pid;
	int err;
	bool matchout;

	/* Close write-fds */
	if (t->output.stdout != NULL)
		close(fdout[1]);
	if (t->output.stderr != NULL)
		close(fderr[1]);

	matchout = test_run_parent_check_outputs(t, fdout[0], fderr[0]);

	/*
	 * break pipe on the other end: either child already closed or we want
	 * to stop it
	 */
	if (t->output.stdout != NULL)
		close(fdout[0]);
	if (t->output.stderr != NULL)
		close(fderr[0]);

	do {
		pid = wait(&err);
		if (pid == -1) {
			ERR("error waitpid(): %m\n");
			return EXIT_FAILURE;
		}
	} while (!WIFEXITED(err) && !WIFSIGNALED(err));

	if (WIFEXITED(err)) {
		if (WEXITSTATUS(err) != 0)
			ERR("'%s' [%u] exited with return code %d\n",
					t->name, pid, WEXITSTATUS(err));
		else
			LOG("'%s' [%u] exited with return code %d\n",
					t->name, pid, WEXITSTATUS(err));
	} else if (WIFSIGNALED(err)) {
		ERR("'%s' [%u] terminated by signal %d (%s)\n", t->name, pid,
				WTERMSIG(err), strsignal(WTERMSIG(err)));
	}

	if (t->expected_fail == false) {
		if (err == 0) {
			if (matchout)
				LOG("%sPASSED%s: %s\n",
					ANSI_HIGHLIGHT_GREEN_ON, ANSI_HIGHLIGHT_OFF,
					t->name);
			else {
				ERR("%sFAILED%s: exit ok but outputs do not match: %s\n",
					ANSI_HIGHLIGHT_RED_ON, ANSI_HIGHLIGHT_OFF,
					t->name);
				err = EXIT_FAILURE;
			}
		} else
			ERR("%sFAILED%s: %s\n",
					ANSI_HIGHLIGHT_RED_ON, ANSI_HIGHLIGHT_OFF,
					t->name);
	} else {
		if (err == 0) {
			if (matchout) {
				LOG("%sUNEXPECTED PASS%s: %s\n",
					ANSI_HIGHLIGHT_RED_ON, ANSI_HIGHLIGHT_OFF,
					t->name);
				err = EXIT_FAILURE;
			} else
				LOG("%sEXPECTED FAIL%s: exit ok but outputs do not match: %s\n",
					ANSI_HIGHLIGHT_GREEN_ON, ANSI_HIGHLIGHT_OFF,
					t->name);
		} else {
			ERR("%sEXPECTED FAIL%s: %s\n",
					ANSI_HIGHLIGHT_GREEN_ON, ANSI_HIGHLIGHT_OFF,
					t->name);
			err = EXIT_SUCCESS;
		}
	}

	return err;
}

static int prepend_path(const char *extra)
{
	char *oldpath, *newpath;
	int r;

	if (extra == NULL)
		return 0;

	oldpath = getenv("PATH");
	if (oldpath == NULL)
		return setenv("PATH", extra, 1);

	if (asprintf(&newpath, "%s:%s", extra, oldpath) < 0) {
		ERR("failed to allocate memory to new PATH\n");
		return -1;
	}

	r = setenv("PATH", newpath, 1);
	free(newpath);

	return r;
}

int test_run(const struct test *t)
{
	pid_t pid;
	int fdout[2];
	int fderr[2];

	if (t->need_spawn && oneshot)
		test_run_spawned(t);

	if (t->output.stdout != NULL) {
		if (pipe(fdout) != 0) {
			ERR("could not create out pipe for %s\n", t->name);
			return EXIT_FAILURE;
		}
	}

	if (t->output.stderr != NULL) {
		if (pipe(fderr) != 0) {
			ERR("could not create err pipe for %s\n", t->name);
			return EXIT_FAILURE;
		}
	}

	if (prepend_path(t->path) < 0) {
		ERR("failed to prepend '%s' to PATH\n", t->path);
		return EXIT_FAILURE;
	}

	LOG("running %s, in forked context\n", t->name);

	pid = fork();
	if (pid < 0) {
		ERR("could not fork(): %m\n");
		LOG("FAILED: %s\n", t->name);
		return EXIT_FAILURE;
	}

	if (pid > 0)
		return test_run_parent(t, fdout, fderr);

	return test_run_child(t, fdout, fderr);
}
