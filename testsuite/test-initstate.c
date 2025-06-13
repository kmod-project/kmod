// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2015  Intel Corporation. All rights reserved.
 */

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

static int test_initstate_from_lookup(void)
{
	struct kmod_ctx *ctx;
	struct kmod_list *list = NULL;
	struct kmod_module *mod;
	const char *null_config = NULL;
	int err, r;

	ctx = kmod_new(NULL, &null_config);
	if (ctx == NULL)
		return EXIT_FAILURE;

	err = kmod_module_new_from_lookup(ctx, "fake-builtin", &list);
	if (err < 0) {
		ERR("could not create module from lookup: %s\n", strerror(-err));
		return EXIT_FAILURE;
	}

	if (!list) {
		ERR("could not create module from lookup: module not found: fake-builtin\n");
		return EXIT_FAILURE;
	}

	mod = kmod_module_get_module(list);

	r = kmod_module_get_initstate(mod);
	if (r != KMOD_MODULE_BUILTIN) {
		ERR("module should have builtin state but is: %s\n",
		    kmod_module_initstate_str(r));
		return EXIT_FAILURE;
	}

	kmod_module_unref(mod);
	kmod_module_unref_list(list);
	kmod_unref(ctx);

	return EXIT_SUCCESS;
}
DEFINE_TEST(
	test_initstate_from_lookup,
	.description =
		"test if libkmod return correct initstate for builtin module from lookup",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-initstate",
		[TC_UNAME_R] = "4.4.4",
	});

static int test_initstate_from_name(void)
{
	struct kmod_ctx *ctx;
	struct kmod_module *mod = NULL;
	const char *null_config = NULL;
	int err, r;

	ctx = kmod_new(NULL, &null_config);
	if (ctx == NULL)
		return EXIT_FAILURE;

	err = kmod_module_new_from_name(ctx, "fake-builtin", &mod);
	if (err != 0) {
		ERR("could not create module from lookup: %s\n", strerror(-err));
		return EXIT_FAILURE;
	}

	if (!mod) {
		ERR("could not create module from lookup: module not found: fake-builtin\n");
		return EXIT_FAILURE;
	}

	r = kmod_module_get_initstate(mod);
	if (r != KMOD_MODULE_BUILTIN) {
		ERR("module should have builtin state but is: %s\n",
		    kmod_module_initstate_str(r));
		return EXIT_FAILURE;
	}

	kmod_module_unref(mod);
	kmod_unref(ctx);

	return EXIT_SUCCESS;
}
DEFINE_TEST(test_initstate_from_name,
	    .description =
		    "test if libkmod return correct initstate for builtin module from name",
	    .config = {
		    [TC_ROOTFS] = TESTSUITE_ROOTFS "test-initstate",
		    [TC_UNAME_R] = "4.4.4",
	    });

TESTSUITE_MAIN();
