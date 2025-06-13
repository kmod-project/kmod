// SPDX-License-Identifier: LGPL-2.1-or-later
/*
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

static int test_remove(void)
{
	struct kmod_ctx *ctx;
	struct kmod_module *mod;
	const char *null_config = NULL;
	int err;
	struct stat st;

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

	err = kmod_module_remove_module(mod, 0);
	if (err != 0) {
		ERR("could not remove module: %s\n", strerror(-err));
		return EXIT_FAILURE;
	}

	if (stat("/sys/module/mod_simple", &st) == 0 && S_ISDIR(st.st_mode)) {
		ERR("could not remove module directory.\n");
		return EXIT_FAILURE;
	}
	kmod_module_unref(mod);
	kmod_unref(ctx);

	return EXIT_SUCCESS;
}
DEFINE_TEST(test_remove,
	    .description = "test if libkmod's delete_module removes module directory",
	    .config = {
		    [TC_ROOTFS] = TESTSUITE_ROOTFS "test-remove/",
		    [TC_INIT_MODULE_RETCODES] = "",
		    [TC_DELETE_MODULE_RETCODES] = "mod_simple:0:0" STRINGIFY(ENOENT),
	    });

TESTSUITE_MAIN();
