/*
 * Copyright (C) 2015 Intel Corporation. All rights reserved.
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

static noreturn int kmod_tool_insert(const struct test *t)
{
	const char *progname = ABS_TOP_BUILDDIR "/tools/kmod";
	const char *const args[] = {
		progname,
		"insert", "mod-simple",
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(kmod_tool_insert,
	.description = "check kmod insert",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-tools/insert",
		[TC_INIT_MODULE_RETCODES] = "",
	},
	.modules_loaded = "mod-simple",
	);

static noreturn int kmod_tool_remove(const struct test *t)
{
	const char *progname = ABS_TOP_BUILDDIR "/tools/kmod";
	const char *const args[] = {
		progname,
		"remove", "mod-simple",
		NULL,
	};

	test_spawn_prog(progname, args);
	exit(EXIT_FAILURE);
}
DEFINE_TEST(kmod_tool_remove,
	.description = "check kmod remove",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-tools/remove",
		[TC_DELETE_MODULE_RETCODES] = "",
	},
	);

TESTSUITE_MAIN();
