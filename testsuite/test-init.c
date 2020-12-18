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

#include <shared/macro.h>

#include <libkmod/libkmod.h>

#include "testsuite.h"

static noreturn int test_load_resources(const struct test *t)
{
	struct kmod_ctx *ctx;
	const char *null_config = NULL;
	int err;

	ctx = kmod_new(NULL, &null_config);
	if (ctx == NULL)
		exit(EXIT_FAILURE);

	kmod_set_log_priority(ctx, 7);

	err = kmod_load_resources(ctx);
	if (err != 0) {
		ERR("could not load libkmod resources: %s\n", strerror(-err));
		exit(EXIT_FAILURE);
	}

	kmod_unref(ctx);

	exit(EXIT_SUCCESS);
}
DEFINE_TEST(test_load_resources,
	    .description = "test if kmod_load_resources works (recent modprobe on kernel without modules.builtin.modinfo)",
	    .config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-init-load-resources/",
		[TC_UNAME_R] = "5.6.0",
	    },
	    .need_spawn = true);

DEFINE_TEST(test_load_resources,
	    .description = "test if kmod_load_resources works with empty modules.builtin.aliases.bin (recent depmod on kernel without modules.builtin.modinfo)",
	    .config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-init-load-resources-empty-builtin-aliases-bin/",
		[TC_UNAME_R] = "5.6.0",
	    },
	    .need_spawn = true);

static noreturn int test_initlib(const struct test *t)
{
	struct kmod_ctx *ctx;
	const char *null_config = NULL;

	ctx = kmod_new(NULL, &null_config);
	if (ctx == NULL)
		exit(EXIT_FAILURE);

	kmod_unref(ctx);

	exit(EXIT_SUCCESS);
}
DEFINE_TEST(test_initlib,
		.description = "test if libkmod's init function work");

static noreturn int test_insert(const struct test *t)
{
	struct kmod_ctx *ctx;
	struct kmod_module *mod;
	const char *null_config = NULL;
	int err;

	ctx = kmod_new(NULL, &null_config);
	if (ctx == NULL)
		exit(EXIT_FAILURE);

	err = kmod_module_new_from_path(ctx, "/mod-simple.ko", &mod);
	if (err != 0) {
		ERR("could not create module from path: %m\n");
		exit(EXIT_FAILURE);
	}

	err = kmod_module_insert_module(mod, 0, NULL);
	if (err != 0) {
		ERR("could not insert module: %m\n");
		exit(EXIT_FAILURE);
	}
	kmod_unref(ctx);

	exit(EXIT_SUCCESS);
}
DEFINE_TEST(test_insert,
	.description = "test if libkmod's insert_module returns ok",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-init/",
		[TC_INIT_MODULE_RETCODES] = "bla:1:20",
	},
	.modules_loaded = "mod_simple",
	.need_spawn = true);

static noreturn int test_remove(const struct test *t)
{
	struct kmod_ctx *ctx;
	struct kmod_module *mod_simple, *mod_bla;
	const char *null_config = NULL;
	int err;

	ctx = kmod_new(NULL, &null_config);
	if (ctx == NULL)
		exit(EXIT_FAILURE);

	err = kmod_module_new_from_name(ctx, "mod-simple", &mod_simple);
	if (err != 0) {
		ERR("could not create module from name: %s\n", strerror(-err));
		exit(EXIT_FAILURE);
	}

	err = kmod_module_new_from_name(ctx, "bla", &mod_bla);
	if (err != 0) {
		ERR("could not create module from name: %s\n", strerror(-err));
		exit(EXIT_FAILURE);
	}

	err = kmod_module_remove_module(mod_simple, 0);
	if (err != 0) {
		ERR("could not remove module: %s\n", strerror(-err));
		exit(EXIT_FAILURE);
	}

	err = kmod_module_remove_module(mod_bla, 0);
	if (err != -ENOENT) {
		ERR("wrong return code for failure test: %d\n", err);
		exit(EXIT_FAILURE);
	}

	kmod_unref(ctx);

	exit(EXIT_SUCCESS);
}
DEFINE_TEST(test_remove,
	.description = "test if libkmod's remove_module returns ok",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-remove/",
		[TC_DELETE_MODULE_RETCODES] =
			"mod-simple:0:0:bla:-1:" STRINGIFY(ENOENT),
	},
	.need_spawn = true);

TESTSUITE_MAIN();
