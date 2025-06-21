// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2012-2013  ProFUSION embedded systems
 */

#include <inttypes.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "testsuite.h"

static int modprobe_show_depends(void)
{
	return EXEC_TOOL(modprobe, "--show-depends", "mod-loop-a");
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

static int modprobe_show_depends2(void)
{
	return EXEC_TOOL(modprobe, "--show-depends", "mod-simple");
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
DEFINE_TEST_WITH_FUNC(modprobe_show_depends_no_load, modprobe_show_depends2,
	.description = "check that --show-depends doesn't load any module",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/show-depends",
	},
	.modules_loaded = "",
	);

static int modprobe_show_alias_to_none(void)
{
	return EXEC_TOOL(modprobe, "--show-depends", "--ignore-install", "mod-simple");
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

static int modprobe_show_exports(void)
{
	return EXEC_TOOL(modprobe, "--show-exports", "/mod-loop-a.ko");
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

static int modprobe_show_exports_module(void)
{
	return EXEC_TOOL(modprobe, "--show-exports", "mod-loop-b");
}
DEFINE_TEST(modprobe_show_exports_module,
	.description = "check if modprobe --show-depends also works with module names",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/show-exports-module",
	},
	.output = {
		.out = TESTSUITE_ROOTFS "test-modprobe/show-exports-module/correct.txt",
		.regex = true,
	});

static int modprobe_builtin(void)
{
	return EXEC_TOOL(modprobe, "unix");
}
DEFINE_TEST(modprobe_builtin, .description = "check if modprobe return 0 for builtin",
	    .config = {
		    [TC_UNAME_R] = "4.4.4",
		    [TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/builtin",
	    });

static int modprobe_builtin_lookup_only(void)
{
	return EXEC_TOOL(modprobe, "-R", "unix");
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

static int modprobe_softdep_loop(void)
{
	return EXEC_TOOL(modprobe, "mod-loop-b");
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

static int modprobe_weakdep_loop(void)
{
	return EXEC_TOOL(modprobe, "mod-loop-b");
}
DEFINE_TEST(modprobe_weakdep_loop,
	.description = "check if modprobe breaks weakdep loop",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/weakdep-loop",
		[TC_INIT_MODULE_RETCODES] = "",
	},
	.modules_loaded = "mod-loop-b",
	.modules_not_loaded = "mod-loop-a,mod-simple-c",
	);

static int modprobe_install_cmd_loop(void)
{
	return EXEC_TOOL(modprobe, "mod-loop-a");
}
DEFINE_TEST(modprobe_install_cmd_loop,
	.description = "check if modprobe breaks install-commands loop",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/install-cmd-loop",
		[TC_INIT_MODULE_RETCODES] = "",
	},
	.env_vars = (const struct keyval[]) {
		{ "MODPROBE", TOOLS_DIR "/modprobe" },
		{ }
		},
	.modules_loaded = "mod-loop-b,mod-loop-a",
	);

static int modprobe_param_kcmdline_show_deps(void)
{
	return EXEC_TOOL(modprobe, "--show-depends", "mod-simple");
}
DEFINE_TEST(modprobe_param_kcmdline_show_deps,
	.description = "check if params from kcmdline are passed to (f)init_module call",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/module-param-kcmdline",
	},
	.output = {
		.out = TESTSUITE_ROOTFS "test-modprobe/module-param-kcmdline/correct.txt",
	});

static int modprobe_param_kcmdline(void)
{
	return EXEC_TOOL(modprobe, "-c");
}
DEFINE_TEST_WITH_FUNC(modprobe_param_kcmdline2, modprobe_param_kcmdline,
	.description = "check if params with no value are parsed correctly from kcmdline",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/module-param-kcmdline2",
	},
	.output = {
		.out = TESTSUITE_ROOTFS "test-modprobe/module-param-kcmdline2/correct.txt",
	});

DEFINE_TEST_WITH_FUNC(modprobe_param_kcmdline3, modprobe_param_kcmdline,
	.description = "check if unrelated strings in kcmdline are correctly ignored",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/module-param-kcmdline3",
	},
	.output = {
		.out = TESTSUITE_ROOTFS "test-modprobe/module-param-kcmdline3/correct.txt",
	});

DEFINE_TEST_WITH_FUNC(modprobe_param_kcmdline4, modprobe_param_kcmdline,
	.description = "check if unrelated strings in kcmdline are correctly ignored",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/module-param-kcmdline4",
	},
	.output = {
		.out = TESTSUITE_ROOTFS "test-modprobe/module-param-kcmdline4/correct.txt",
	});

DEFINE_TEST_WITH_FUNC(modprobe_param_kcmdline5, modprobe_param_kcmdline,
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

DEFINE_TEST_WITH_FUNC(modprobe_param_kcmdline6, modprobe_param_kcmdline,
	.description = "check if dots on other parts of kcmdline don't confuse our parser",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/module-param-kcmdline6",
	},
	.output = {
		.out = TESTSUITE_ROOTFS "test-modprobe/module-param-kcmdline6/correct.txt",
	});

DEFINE_TEST_WITH_FUNC(modprobe_param_kcmdline7, modprobe_param_kcmdline,
	.description = "check if dots on other parts of kcmdline don't confuse our parser",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/module-param-kcmdline7",
	},
	.output = {
		.out = TESTSUITE_ROOTFS "test-modprobe/module-param-kcmdline7/correct.txt",
	});

DEFINE_TEST_WITH_FUNC(modprobe_param_kcmdline8, modprobe_param_kcmdline,
	.description = "check if dots on other parts of kcmdline don't confuse our parser",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/module-param-kcmdline8",
	},
	.output = {
		.out = TESTSUITE_ROOTFS "test-modprobe/module-param-kcmdline8/correct.txt",
	});

DEFINE_TEST_WITH_FUNC(modprobe_param_kcmdline9, modprobe_param_kcmdline,
	.description = "check if multiple blacklists are parsed correctly",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/module-param-kcmdline9",
	},
	.output = {
		.out = TESTSUITE_ROOTFS "test-modprobe/module-param-kcmdline9/correct.txt",
	});

static int modprobe_force(void)
{
	return EXEC_TOOL(modprobe, "--force", "mod-simple");
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

static int modprobe_force_modversion(void)
{
	return EXEC_TOOL(modprobe, "--force-modversion", "mod-simple");
}
DEFINE_TEST(modprobe_force_modversion,
	.description = "check modprobe --force-modversion",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/force-modversion",
		[TC_INIT_MODULE_RETCODES] = "",
	},
	.modules_loaded = "mod-simple",
	);

static int modprobe_force_vermagic(void)
{
	return EXEC_TOOL(modprobe, "--force-vermagic", "mod-simple");
}
DEFINE_TEST(modprobe_force_vermagic,
	.description = "check modprobe --force-vermagic",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/force-vermagic",
		[TC_INIT_MODULE_RETCODES] = "",
	},
	.modules_loaded = "mod-simple",
	);

static int modprobe_oldkernel(void)
{
	return EXEC_TOOL(modprobe, "mod-simple");
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

static int modprobe_oldkernel_force(void)
{
	return EXEC_TOOL(modprobe, "--force", "mod-simple");
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

static int modprobe_oldkernel_force_modversion(void)
{
	return EXEC_TOOL(modprobe, "--force-modversion", "mod-simple");
}
DEFINE_TEST(modprobe_oldkernel_force_modversion,
	.description = "check modprobe --force-modversion with kernel without finit_module()",
	.config = {
		[TC_UNAME_R] = "3.3.3",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/oldkernel-force-modversion",
		[TC_INIT_MODULE_RETCODES] = "",
	},
	.modules_loaded = "mod-simple",
	);

static int modprobe_oldkernel_force_vermagic(void)
{
	return EXEC_TOOL(modprobe, "--force-vermagic", "mod-simple");
}
DEFINE_TEST(modprobe_oldkernel_force_vermagic,
	.description = "check modprobe --force-vermagic with kernel without finit_module()",
	.config = {
		[TC_UNAME_R] = "3.3.3",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/oldkernel-force-vermagic",
		[TC_INIT_MODULE_RETCODES] = "",
	},
	.modules_loaded = "mod-simple",
	);

static int modprobe_external(void)
{
	return EXEC_TOOL(modprobe, "mod-simple");
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

static int modprobe_module_from_abspath(void)
{
	return EXEC_TOOL(modprobe, "/home/foo/mod-simple.ko");
}
DEFINE_TEST(modprobe_module_from_abspath,
	.description = "check modprobe able to load module given as an absolute path",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/module-from-abspath",
		[TC_INIT_MODULE_RETCODES] = "",
	},
	.modules_loaded = "mod-simple",
	);

static int modprobe_module_from_relpath(void)
{
	if (chdir("/home/foo") != 0) {
		perror("failed to change into /home/foo");
		return EXIT_FAILURE;
	}

	return EXEC_TOOL(modprobe, "./mod-simple.ko");
}
DEFINE_TEST(modprobe_module_from_relpath,
	.description = "check modprobe able to load module given as a relative path",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-modprobe/module-from-relpath",
		[TC_INIT_MODULE_RETCODES] = "",
	},
	.modules_loaded = "mod-simple",
	);

TESTSUITE_MAIN();
