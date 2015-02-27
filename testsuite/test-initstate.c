/*
 * Copyright (C) 2015  Intel Corporation. All rights reserved.
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
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <libkmod/libkmod.h>

#include <shared/macro.h>

#include "testsuite.h"


static noreturn int test_initstate_from_lookup(const struct test *t)
{
	struct kmod_ctx *ctx;
	struct kmod_list *list = NULL;
	struct kmod_module *mod;
	const char *null_config = NULL;
	int err, r;

	ctx = kmod_new(NULL, &null_config);
	if (ctx == NULL)
		exit(EXIT_FAILURE);

	err = kmod_module_new_from_lookup(ctx, "fake-builtin", &list);
	if (err != 0) {
		ERR("could not create module from lookup: %s\n", strerror(-err));
		exit(EXIT_FAILURE);
	}

	if (!list) {
		ERR("could not create module from lookup: module not found: fake-builtin\n");
		exit(EXIT_FAILURE);
	}

	mod = kmod_module_get_module(list);

	r = kmod_module_get_initstate(mod);
	if (r != KMOD_MODULE_BUILTIN) {
		ERR("module should have builtin state but is: %s\n",
		    kmod_module_initstate_str(r));
		exit(EXIT_FAILURE);
	}

	kmod_module_unref(mod);
	kmod_module_unref_list(list);
	kmod_unref(ctx);

	exit(EXIT_SUCCESS);
}
DEFINE_TEST(test_initstate_from_lookup,
	.description = "test if libkmod return correct initstate for builtin module from lookup",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-initstate",
		[TC_UNAME_R] = "4.4.4",
	},
	.need_spawn = true);

static noreturn int test_initstate_from_name(const struct test *t)
{
	struct kmod_ctx *ctx;
	struct kmod_module *mod = NULL;
	const char *null_config = NULL;
	int err, r;

	ctx = kmod_new(NULL, &null_config);
	if (ctx == NULL)
		exit(EXIT_FAILURE);

	err = kmod_module_new_from_name(ctx, "fake-builtin", &mod);
	if (err != 0) {
		ERR("could not create module from lookup: %s\n", strerror(-err));
		exit(EXIT_FAILURE);
	}

	if (!mod) {
		ERR("could not create module from lookup: module not found: fake-builtin\n");
		exit(EXIT_FAILURE);
	}

	r = kmod_module_get_initstate(mod);
	if (r != KMOD_MODULE_BUILTIN) {
		ERR("module should have builtin state but is: %s\n",
		    kmod_module_initstate_str(r));
		exit(EXIT_FAILURE);
	}

	kmod_module_unref(mod);
	kmod_unref(ctx);

	exit(EXIT_SUCCESS);
}
DEFINE_TEST(test_initstate_from_name,
	.description = "test if libkmod return correct initstate for builtin module from name",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-initstate",
		[TC_UNAME_R] = "4.4.4",
	},
	.need_spawn = true);




TESTSUITE_MAIN();
