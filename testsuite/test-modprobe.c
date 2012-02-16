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

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>

#include "testsuite.h"

static __noreturn int modprobe_show_depends(const struct test *t)
{
	const char *progname = ABS_TOP_BUILDDIR "/tools/modprobe";
	const char *const args[] = {
		progname,
		"--show-depends", "btusb",
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
static DEFINE_TEST(modprobe_show_depends,
	.description = "check if output for modprobe --show-depends is correct for loaded modules",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/show-depends",
	},
	.output = {
		.stdout = TESTSUITE_ROOTFS "test-modprobe/show-depends/correct.txt",
	});

static __noreturn int modprobe_show_depends2(const struct test *t)
{
	const char *progname = ABS_TOP_BUILDDIR "/tools/modprobe";
	const char *const args[] = {
		progname,
		"--show-depends", "psmouse",
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
static DEFINE_TEST(modprobe_show_depends2,
	.description = "check if output for modprobe --show-depends is correct",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/show-depends",
	},
	.output = {
		.stdout = TESTSUITE_ROOTFS "test-modprobe/show-depends/correct-psmouse.txt",
	});

static __noreturn int modprobe_builtin(const struct test *t)
{
	const char *progname = ABS_TOP_BUILDDIR "/tools/modprobe";
	const char *const args[] = {
		progname,
		"unix",
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
static DEFINE_TEST(modprobe_builtin,
	.description = "check if modprobe return 0 for builtin",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/builtin",
	});

static const struct test *tests[] = {
	&smodprobe_show_depends,
	&smodprobe_show_depends2,
	&smodprobe_builtin,
	NULL,
};

TESTSUITE_MAIN(tests);
