/*
 * Copyright (C) 2012-2013  ProFUSION embedded systems
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
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
		char *ldpreload = getenv("LD_PRELOAD");
		ERR("u.release=%s should be %s\n", u.release, TEST_UNAME);
		ERR("LD_PRELOAD=%s\n", ldpreload);
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}
DEFINE_TEST(testsuite_uname,
	.description = "test if trap to uname() works",
	.config = {
		[TC_UNAME_R] = TEST_UNAME,
	},
	.need_spawn = true);

static int testsuite_rootfs_fopen(const struct test *t)
{
	FILE *fp;
	char s[100];
	int n;

	fp = fopen("/lib/modules/a", "r");
	if (fp == NULL)
		return EXIT_FAILURE;;

	n = fscanf(fp, "%s", s);
	if (n != 1)
		return EXIT_FAILURE;

	if (!streq(s, "kmod-test-chroot-works"))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
DEFINE_TEST(testsuite_rootfs_fopen,
	.description = "test if rootfs works - fopen()",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-rootfs/",
	},
	.need_spawn = true);

static int testsuite_rootfs_open(const struct test *t)
{
	char buf[100];
	int fd, done;

	fd = open("/lib/modules/a", O_RDONLY);
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
DEFINE_TEST(testsuite_rootfs_open,
	.description = "test if rootfs works - open()",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-rootfs/",
	},
	.need_spawn = true);

static int testsuite_rootfs_stat_access(const struct test *t)
{
	struct stat st;

	if (access("/lib/modules/a", F_OK) < 0) {
		ERR("access failed: %m\n");
		return EXIT_FAILURE;
	}

	if (stat("/lib/modules/a", &st) < 0) {
		ERR("stat failed: %m\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
DEFINE_TEST(testsuite_rootfs_stat_access,
	.description = "test if rootfs works - stat() and access()",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-rootfs/",
	},
	.need_spawn = true);

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
DEFINE_TEST(testsuite_rootfs_opendir,
	.description = "test if rootfs works - opendir()",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-rootfs/",
	},
	.need_spawn = true);

TESTSUITE_MAIN();
