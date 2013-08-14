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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
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


static __noreturn int modprobe_show_alias_to_none(const struct test *t)
{
	const char *progname = ABS_TOP_BUILDDIR "/tools/modprobe";
	const char *const args[] = {
		progname,
		"--show-depends", "--ignore-install", "--quiet", "psmouse",
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
static DEFINE_TEST(modprobe_show_alias_to_none,
	.description = "check if modprobe --show-depends doesn't explode with an alias to nothing",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/alias-to-none",
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

static __noreturn int modprobe_softdep_loop(const struct test *t)
{
	const char *progname = ABS_TOP_BUILDDIR "/tools/modprobe";
	const char *const args[] = {
		progname,
		"bluetooth",
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
static DEFINE_TEST(modprobe_softdep_loop,
	.description = "check if modprobe breaks softdep loop",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/softdep-loop",
		[TC_INIT_MODULE_RETCODES] = "",
	});

static __noreturn int modprobe_install_cmd_loop(const struct test *t)
{
	const char *progname = ABS_TOP_BUILDDIR "/tools/modprobe";
	const char *const args[] = {
		progname,
		"snd-pcm",
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
static DEFINE_TEST(modprobe_install_cmd_loop,
	.description = "check if modprobe breaks install-commands loop",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/install-cmd-loop",
		[TC_INIT_MODULE_RETCODES] = "",
	},
	.env_vars = (const struct keyval[]) {
		{ "MODPROBE", ABS_TOP_BUILDDIR "/tools/modprobe" },
		{ }
		},
	);

static __noreturn int modprobe_param_kcmdline(const struct test *t)
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
static DEFINE_TEST(modprobe_param_kcmdline,
	.description = "check if params are parsed correctly from kcmdline",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/module-param-kcmdline",
	},
	.output = {
		.stdout = TESTSUITE_ROOTFS "test-modprobe/module-param-kcmdline/correct.txt",
	});


static const struct test *tests[] = {
	&smodprobe_show_depends,
	&smodprobe_show_depends2,
	&smodprobe_show_alias_to_none,
	&smodprobe_builtin,
	&smodprobe_softdep_loop,
	&smodprobe_install_cmd_loop,
	&smodprobe_param_kcmdline,
	NULL,
};

TESTSUITE_MAIN(tests);
