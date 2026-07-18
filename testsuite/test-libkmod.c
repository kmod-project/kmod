// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2012-2013  ProFUSION embedded systems
 * Copyright © 2025 Intel Corporation
 */

#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>

#include <shared/macro.h>

#include <libkmod/libkmod.h>

#include "testsuite.h"

static int test_load_resources(void)
{
	struct kmod_ctx *ctx;
	const char *null_config = NULL;
	int err;

	ctx = kmod_new(NULL, &null_config);
	TS_ASSERT(ctx != NULL);

	kmod_set_log_priority(ctx, 7);

	err = kmod_load_resources(ctx);
	TS_ASSERT(err == 0);

	kmod_unref(ctx);

	return 0;
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
	TS_ASSERT(ctx != NULL);

	kmod_unref(ctx);

	return 0;
}
DEFINE_TEST(test_initlib, .description = "test if libkmod's init function work");

static int test_insert(void)
{
	struct kmod_ctx *ctx;
	struct kmod_module *mod;
	const char *null_config = NULL;
	int err;

	ctx = kmod_new(NULL, &null_config);
	TS_ASSERT(ctx != NULL);

	err = kmod_module_new_from_path(ctx, "/mod-simple.ko", &mod);
	TS_ASSERT(err == 0);

	err = kmod_module_insert_module(mod, 0, NULL);
	TS_ASSERT(err == 0);

	kmod_module_unref(mod);
	kmod_unref(ctx);

	return 0;
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
	TS_ASSERT(ctx != NULL);

	err = kmod_module_new_from_name(ctx, "mod-simple", &mod_simple);
	TS_ASSERT(err == 0);

	err = kmod_module_new_from_name(ctx, "bla", &mod_bla);
	TS_ASSERT(err == 0);

	err = kmod_module_remove_module(mod_simple, 0);
	TS_ASSERT(err == 0);

	err = kmod_module_remove_module(mod_bla, 0);
	TS_ASSERT(err == -ENOENT);

	kmod_module_unref(mod_bla);
	kmod_module_unref(mod_simple);
	kmod_unref(ctx);

	return 0;
}
DEFINE_TEST(
	test_remove, .description = "test if libkmod's remove_module returns ok",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-remove/",
		[TC_DELETE_MODULE_RETCODES] = "mod-simple:0:0:bla:-1:" STRINGIFY(ENOENT),
	});

static int test_remove2(void)
{
	struct kmod_ctx *ctx;
	struct kmod_module *mod;
	const char *null_config = NULL;
	int err;
	struct stat st;

	ctx = kmod_new(NULL, &null_config);
	TS_ASSERT(ctx != NULL);

	err = kmod_module_new_from_path(ctx, "/mod-simple.ko", &mod);
	TS_ASSERT(err == 0);

	err = kmod_module_insert_module(mod, 0, NULL);
	TS_ASSERT(err == 0);

	err = kmod_module_remove_module(mod, 0);
	TS_ASSERT(err == 0);

	TS_ASSERT(stat("/sys/module/mod_simple", &st) != 0 || !S_ISDIR(st.st_mode));

	kmod_module_unref(mod);
	kmod_unref(ctx);

	return 0;
}
DEFINE_TEST(test_remove2,
	    .description = "test if libkmod's delete_module removes module directory",
	    .config = {
		    [TC_ROOTFS] = TESTSUITE_ROOTFS "test-remove2/",
		    [TC_INIT_MODULE_RETCODES] = "",
		    [TC_DELETE_MODULE_RETCODES] = "mod_simple:0:0" STRINGIFY(ENOENT),
	    });

TESTSUITE_MAIN();
