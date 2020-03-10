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

#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "testsuite.h"

static noreturn int modprobe_show_depends(const struct test *t)
{
	const char *progname = ABS_TOP_BUILDDIR "/tools/modprobe";
	const char *const args[] = {
		progname,
		"--show-depends", "mod-loop-a",
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(modprobe_show_depends,
	.description = "check if output for modprobe --show-depends is correct for loaded modules",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/show-depends",
	},
	.output = {
		.out = TESTSUITE_ROOTFS "test-modprobe/show-depends/correct.txt",
	});

static noreturn int modprobe_show_depends2(const struct test *t)
{
	const char *progname = ABS_TOP_BUILDDIR "/tools/modprobe";
	const char *const args[] = {
		progname,
		"--show-depends", "mod-simple",
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(modprobe_show_depends2,
	.description = "check if output for modprobe --show-depends is correct",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/show-depends",
	},
	.output = {
		.out = TESTSUITE_ROOTFS "test-modprobe/show-depends/correct-mod-simple.txt",
	});


static noreturn int modprobe_show_alias_to_none(const struct test *t)
{
	const char *progname = ABS_TOP_BUILDDIR "/tools/modprobe";
	const char *const args[] = {
		progname,
		"--show-depends", "--ignore-install", "--quiet", "mod-simple",
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(modprobe_show_alias_to_none,
	.description = "check if modprobe --show-depends doesn't explode with an alias to nothing",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/alias-to-none",
	},
	.output = {
		.out = TESTSUITE_ROOTFS "test-modprobe/alias-to-none/correct.txt",
	},
	.modules_loaded = "",
	);


static noreturn int modprobe_show_exports(const struct test *t)
{
	const char *progname = ABS_TOP_BUILDDIR "/tools/modprobe";
	const char *const args[] = {
		progname,
		"--show-exports", "--quiet", "/mod-loop-a.ko",
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(modprobe_show_exports,
	.description = "check if modprobe --show-depends doesn't explode with an alias to nothing",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/show-exports",
	},
	.output = {
		.out = TESTSUITE_ROOTFS "test-modprobe/show-exports/correct.txt",
		.regex = true,
	});


static noreturn int modprobe_builtin(const struct test *t)
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
DEFINE_TEST(modprobe_builtin,
	.description = "check if modprobe return 0 for builtin",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/builtin",
	});

static noreturn int modprobe_builtin_lookup_only(const struct test *t)
{
	const char *progname = ABS_TOP_BUILDDIR "/tools/modprobe";
	const char *const args[] = {
		progname,
		"-R", "unix",
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(modprobe_builtin_lookup_only,
	.description = "check if modprobe -R correctly returns the builtin module",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/builtin",
	},
	.output = {
		.out = TESTSUITE_ROOTFS "test-modprobe/builtin/correct.txt",
	});

static noreturn int modprobe_softdep_loop(const struct test *t)
{
	const char *progname = ABS_TOP_BUILDDIR "/tools/modprobe";
	const char *const args[] = {
		progname,
		"mod-loop-b",
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(modprobe_softdep_loop,
	.description = "check if modprobe breaks softdep loop",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/softdep-loop",
		[TC_INIT_MODULE_RETCODES] = "",
	},
	.modules_loaded = "mod-loop-a,mod-loop-b",
	);

static noreturn int modprobe_install_cmd_loop(const struct test *t)
{
	const char *progname = ABS_TOP_BUILDDIR "/tools/modprobe";
	const char *const args[] = {
		progname,
		"mod-loop-a",
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(modprobe_install_cmd_loop,
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
	.modules_loaded = "mod-loop-b,mod-loop-a",
	);

static noreturn int modprobe_param_kcmdline(const struct test *t)
{
	const char *progname = ABS_TOP_BUILDDIR "/tools/modprobe";
	const char *const args[] = {
		progname,
		"--show-depends", "mod-simple",
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(modprobe_param_kcmdline,
	.description = "check if params from kcmdline are passed to (f)init_module call",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/module-param-kcmdline",
	},
	.output = {
		.out = TESTSUITE_ROOTFS "test-modprobe/module-param-kcmdline/correct.txt",
	},
	.modules_loaded = "",
	);

static noreturn int modprobe_param_kcmdline2(const struct test *t)
{
	const char *progname = ABS_TOP_BUILDDIR "/tools/modprobe";
	const char *const args[] = {
		progname,
		"-c",
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(modprobe_param_kcmdline2,
	.description = "check if params with no value are parsed correctly from kcmdline",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/module-param-kcmdline2",
	},
	.output = {
		.out = TESTSUITE_ROOTFS "test-modprobe/module-param-kcmdline2/correct.txt",
	},
	.modules_loaded = "",
	);

static noreturn int modprobe_param_kcmdline3(const struct test *t)
{
	const char *progname = ABS_TOP_BUILDDIR "/tools/modprobe";
	const char *const args[] = {
		progname,
		"-c",
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(modprobe_param_kcmdline3,
	.description = "check if unrelated strings in kcmdline are correctly ignored",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/module-param-kcmdline3",
	},
	.output = {
		.out = TESTSUITE_ROOTFS "test-modprobe/module-param-kcmdline3/correct.txt",
	},
	.modules_loaded = "",
	);

static noreturn int modprobe_param_kcmdline4(const struct test *t)
{
	const char *progname = ABS_TOP_BUILDDIR "/tools/modprobe";
	const char *const args[] = {
		progname,
		"-c",
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(modprobe_param_kcmdline4,
	.description = "check if unrelated strings in kcmdline are correctly ignored",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/module-param-kcmdline4",
	},
	.output = {
		.out = TESTSUITE_ROOTFS "test-modprobe/module-param-kcmdline4/correct.txt",
	},
	.modules_loaded = "",
	);

static noreturn int modprobe_param_kcmdline5(const struct test *t)
{
	const char *progname = ABS_TOP_BUILDDIR "/tools/modprobe";
	const char *const args[] = {
		progname,
		"-c",
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(modprobe_param_kcmdline5,
	.description = "check if params with spaces are parsed correctly from kcmdline",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/module-param-kcmdline5",
	},
	.output = {
		.out = TESTSUITE_ROOTFS "test-modprobe/module-param-kcmdline5/correct.txt",
	},
	.modules_loaded = "",
	);


static noreturn int modprobe_param_kcmdline6(const struct test *t)
{
	const char *progname = ABS_TOP_BUILDDIR "/tools/modprobe";
	const char *const args[] = {
		progname,
		"-c",
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(modprobe_param_kcmdline6,
	.description = "check if dots on other parts of kcmdline don't confuse our parser",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/module-param-kcmdline6",
	},
	.output = {
		.out = TESTSUITE_ROOTFS "test-modprobe/module-param-kcmdline6/correct.txt",
	},
	.modules_loaded = "",
	);


static noreturn int modprobe_force(const struct test *t)
{
	const char *progname = ABS_TOP_BUILDDIR "/tools/modprobe";
	const char *const args[] = {
		progname,
		"--force", "mod-simple",
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(modprobe_force,
	.description = "check modprobe --force",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/force",
		[TC_INIT_MODULE_RETCODES] = "",
	},
	.modules_loaded = "mod-simple",
	);

static noreturn int modprobe_oldkernel(const struct test *t)
{
	const char *progname = ABS_TOP_BUILDDIR "/tools/modprobe";
	const char *const args[] = {
		progname,
		"mod-simple",
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(modprobe_oldkernel,
	.description = "check modprobe with kernel without finit_module()",
	.config = {
		[TC_UNAME_R] = "3.3.3",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/oldkernel",
		[TC_INIT_MODULE_RETCODES] = "",
	},
	.modules_loaded = "mod-simple",
	);

static noreturn int modprobe_oldkernel_force(const struct test *t)
{
	const char *progname = ABS_TOP_BUILDDIR "/tools/modprobe";
	const char *const args[] = {
		progname,
		"--force", "mod-simple",
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(modprobe_oldkernel_force,
	.description = "check modprobe --force with kernel without finit_module()",
	.config = {
		[TC_UNAME_R] = "3.3.3",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/oldkernel-force",
		[TC_INIT_MODULE_RETCODES] = "",
	},
	.modules_loaded = "mod-simple",
	);

static noreturn int modprobe_external(const struct test *t)
{
	const char *progname = ABS_TOP_BUILDDIR "/tools/modprobe";
	const char *const args[] = {
		progname,
		"mod-simple",
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(modprobe_external,
	.description = "check modprobe able to load external module",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/external",
		[TC_INIT_MODULE_RETCODES] = "",
	},
	.modules_loaded = "mod-simple",
	);

TESTSUITE_MAIN();
