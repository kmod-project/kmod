// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2012-2013  ProFUSION embedded systems
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

static int test_load_resources(void)
{
	struct kmod_ctx *ctx;
	const char *null_config = NULL;
	int err;

	ctx = kmod_new(NULL, &null_config);
	if (ctx == NULL)
		return EXIT_FAILURE;

	kmod_set_log_priority(ctx, 7);

	err = kmod_load_resources(ctx);
	if (err != 0) {
		ERR("could not load libkmod resources: %s\n", strerror(-err));
		return EXIT_FAILURE;
	}

	kmod_unref(ctx);

	return EXIT_SUCCESS;
}
DEFINE_TEST_WITH_FUNC(
	test_load_resource1, test_load_resources,
	.description =
		"test if kmod_load_resources works (recent modprobe on kernel without modules.builtin.modinfo)",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-init-load-resources/",
		[TC_UNAME_R] = "5.6.0",
	});

DEFINE_TEST_WITH_FUNC(
	test_load_resource2, test_load_resources,
	.description =
		"test if kmod_load_resources works with empty modules.builtin.aliases.bin (recent depmod on kernel without modules.builtin.modinfo)",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS
		"test-init-load-resources-empty-builtin-aliases-bin/",
		[TC_UNAME_R] = "5.6.0",
	});

static int test_initlib(void)
{
	struct kmod_ctx *ctx;
	const char *null_config = NULL;

	ctx = kmod_new(NULL, &null_config);
	if (ctx == NULL)
		return EXIT_FAILURE;

	kmod_unref(ctx);

	return EXIT_SUCCESS;
}
DEFINE_TEST(test_initlib, .description = "test if libkmod's init function work");

static int test_insert(void)
{
	struct kmod_ctx *ctx;
	struct kmod_module *mod;
	const char *null_config = NULL;
	int err;

	ctx = kmod_new(NULL, &null_config);
	if (ctx == NULL)
		return EXIT_FAILURE;

	err = kmod_module_new_from_path(ctx, "/mod-simple.ko", &mod);
	if (err != 0) {
		ERR("could not create module from path: %s\n", strerror(-err));
		return EXIT_FAILURE;
	}

	err = kmod_module_insert_module(mod, 0, NULL);
	if (err != 0) {
		ERR("could not insert module: %s\n", strerror(-err));
		return EXIT_FAILURE;
	}
	kmod_module_unref(mod);
	kmod_unref(ctx);

	return EXIT_SUCCESS;
}
DEFINE_TEST(test_insert,
	.description = "test if libkmod's insert_module returns ok",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-init/",
		[TC_INIT_MODULE_RETCODES] = "bla:1:20",
	},
	.modules_loaded = "mod_simple");

static int test_remove(void)
{
	struct kmod_ctx *ctx;
	struct kmod_module *mod_simple, *mod_bla;
	const char *null_config = NULL;
	int err;

	ctx = kmod_new(NULL, &null_config);
	if (ctx == NULL)
		return EXIT_FAILURE;

	err = kmod_module_new_from_name(ctx, "mod-simple", &mod_simple);
	if (err != 0) {
		ERR("could not create module from name: %s\n", strerror(-err));
		return EXIT_FAILURE;
	}

	err = kmod_module_new_from_name(ctx, "bla", &mod_bla);
	if (err != 0) {
		ERR("could not create module from name: %s\n", strerror(-err));
		return EXIT_FAILURE;
	}

	err = kmod_module_remove_module(mod_simple, 0);
	if (err != 0) {
		ERR("could not remove module: %s\n", strerror(-err));
		return EXIT_FAILURE;
	}

	err = kmod_module_remove_module(mod_bla, 0);
	if (err != -ENOENT) {
		ERR("wrong return code for failure test: %d\n", err);
		return EXIT_FAILURE;
	}

	kmod_module_unref(mod_bla);
	kmod_module_unref(mod_simple);
	kmod_unref(ctx);

	return EXIT_SUCCESS;
}
DEFINE_TEST(
	test_remove, .description = "test if libkmod's remove_module returns ok",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-remove/",
		[TC_DELETE_MODULE_RETCODES] = "mod-simple:0:0:bla:-1:" STRINGIFY(ENOENT),
	});

TESTSUITE_MAIN();
