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
static int testsuite_uname(void)
{
	struct utsname u;
	int err = uname(&u);

	TS_ASSERT(err == 0);

	TS_ASSERT(streq(u.release, TEST_UNAME));

	return 0;
}
DEFINE_TEST(testsuite_uname, .description = "test if trap to uname() works",
	    .config = {
		    [TC_UNAME_R] = TEST_UNAME,
	    });

static int testsuite_rootfs_fopen(void)
{
	FILE *fp;
	char s[100];
	int n;

	fp = fopen(MODULE_DIRECTORY "/a", "r");
	TS_ASSERT(fp != NULL);

	n = fscanf(fp, "%s", s);
	TS_ASSERT(n == 1);

	TS_ASSERT(streq(s, "kmod-test-chroot-works"));

	return 0;
}
DEFINE_TEST(testsuite_rootfs_fopen, .description = "test if rootfs works - fopen()",
	    .config = {
		    [TC_ROOTFS] = TESTSUITE_ROOTFS "test-rootfs/",
	    });

static int testsuite_rootfs_open(void)
{
	char buf[100];
	int fd, done;

	fd = open(MODULE_DIRECTORY "/a", O_RDONLY);
	TS_ASSERT(fd >= 0);

	for (done = 0;;) {
		int r = read(fd, buf + done, sizeof(buf) - 1 - done);
		if (r == 0)
			break;
		if (r == -EAGAIN)
			continue;

		done += r;
	}

	buf[done] = '\0';

	TS_ASSERT(streq(buf, "kmod-test-chroot-works\n"));

	return 0;
}
DEFINE_TEST(testsuite_rootfs_open, .description = "test if rootfs works - open()",
	    .config = {
		    [TC_ROOTFS] = TESTSUITE_ROOTFS "test-rootfs/",
	    });

static int testsuite_rootfs_stat(void)
{
	struct stat st;

	TS_ASSERT(stat(MODULE_DIRECTORY "/a", &st) == 0);

	return 0;
}
DEFINE_TEST(testsuite_rootfs_stat, .description = "test if rootfs works - stat()",
	    .config = {
		    [TC_ROOTFS] = TESTSUITE_ROOTFS "test-rootfs/",
	    });

static int testsuite_rootfs_opendir(void)
{
	DIR *d;

	d = opendir("/testdir");
	TS_ASSERT(d != NULL);

	closedir(d);
	return 0;
}
DEFINE_TEST(testsuite_rootfs_opendir, .description = "test if rootfs works - opendir()",
	    .config = {
		    [TC_ROOTFS] = TESTSUITE_ROOTFS "test-rootfs/",
	    });

TESTSUITE_MAIN();
