// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2012-2013  ProFUSION embedded systems
 */

#include <dirent.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>

#include <shared/util.h>

#include <libkmod/libkmod.h>

#include "testsuite.h"

#define TEST_UNAME "4.0.20-kmod"
static noreturn int testsuite_uname(const struct test *t)
{
	struct utsname u;
	int err = uname(&u);

	if (err < 0)
		exit(EXIT_FAILURE);

	if (!streq(u.release, TEST_UNAME)) {
		const char *ldpreload = getenv("LD_PRELOAD");
		ERR("u.release=%s should be %s\n", u.release, TEST_UNAME);
		ERR("LD_PRELOAD=%s\n", ldpreload);
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}
DEFINE_TEST(testsuite_uname, .description = "test if trap to uname() works",
	    .config = {
		    [TC_UNAME_R] = TEST_UNAME,
	    });

static int testsuite_rootfs_fopen(const struct test *t)
{
	FILE *fp;
	char s[100];
	int n;

	fp = fopen(MODULE_DIRECTORY "/a", "r");
	if (fp == NULL)
		return EXIT_FAILURE;

	n = fscanf(fp, "%s", s);
	if (n != 1)
		return EXIT_FAILURE;

	if (!streq(s, "kmod-test-chroot-works"))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
DEFINE_TEST(testsuite_rootfs_fopen, .description = "test if rootfs works - fopen()",
	    .config = {
		    [TC_ROOTFS] = "test-rootfs/",
	    });

static int testsuite_rootfs_open(const struct test *t)
{
	char buf[100];
	int fd, done;

	fd = open(MODULE_DIRECTORY "/a", O_RDONLY);
	if (fd < 0)
		return EXIT_FAILURE;

	for (done = 0;;) {
		int r = read(fd, buf + done, sizeof(buf) - 1 - done);
		if (r == 0)
			break;
		if (r == -EAGAIN)
			continue;

		done += r;
	}

	buf[done] = '\0';

	if (!streq(buf, "kmod-test-chroot-works\n"))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
DEFINE_TEST(testsuite_rootfs_open, .description = "test if rootfs works - open()",
	    .config = {
		    [TC_ROOTFS] = "test-rootfs/",
	    });

static int testsuite_rootfs_stat(const struct test *t)
{
	struct stat st;

	if (stat(MODULE_DIRECTORY "/a", &st) < 0) {
		ERR("stat failed: %m\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
DEFINE_TEST(testsuite_rootfs_stat, .description = "test if rootfs works - stat()",
	    .config = {
		    [TC_ROOTFS] = "test-rootfs/",
	    });

static int testsuite_rootfs_opendir(const struct test *t)
{
	DIR *d;

	d = opendir("/testdir");
	if (d == NULL) {
		ERR("opendir failed: %m\n");
		return EXIT_FAILURE;
	}

	closedir(d);
	return EXIT_SUCCESS;
}
DEFINE_TEST(testsuite_rootfs_opendir, .description = "test if rootfs works - opendir()",
	    .config = {
		    [TC_ROOTFS] = "test-rootfs/",
	    });

TESTSUITE_MAIN();
