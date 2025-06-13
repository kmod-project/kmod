// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2012-2013  ProFUSION embedded systems
 */

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libkmod/libkmod.h>

#include "testsuite.h"

static int from_name(void)
{
	static const char *const modnames[] = {
		// clang-format off
		"ext4",
		"balbalbalbbalbalbalbalbalbalbal",
		"snd-hda-intel",
		"snd-timer",
		"iTCO_wdt",
		// clang-format on
	};
	struct kmod_ctx *ctx;
	struct kmod_module *mod;
	const char *null_config = NULL;
	int err;

	ctx = kmod_new(NULL, &null_config);
	if (ctx == NULL)
		return EXIT_FAILURE;

	for (size_t i = 0; i < ARRAY_SIZE(modnames); i++) {
		err = kmod_module_new_from_name(ctx, modnames[i], &mod);
		if (err < 0)
			return EXIT_FAILURE;

		printf("modname: %s\n", kmod_module_get_name(mod));
		kmod_module_unref(mod);
	}

	kmod_unref(ctx);

	return EXIT_SUCCESS;
}
DEFINE_TEST(from_name,
	.description = "check if module names are parsed correctly",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-new-module/from_name/",
	},
	.output = {
		.out = TESTSUITE_ROOTFS "test-new-module/from_name/correct.txt",
	});

static int from_alias(void)
{
	static const char *const modnames[] = {
		"ext4.*",
	};
	struct kmod_ctx *ctx;
	int err;

	ctx = kmod_new(NULL, NULL);
	if (ctx == NULL)
		return EXIT_FAILURE;

	for (size_t i = 0; i < ARRAY_SIZE(modnames); i++) {
		struct kmod_list *l, *list = NULL;

		err = kmod_module_new_from_lookup(ctx, modnames[i], &list);
		if (err < 0)
			return EXIT_FAILURE;

		kmod_list_foreach(l, list) {
			struct kmod_module *m;
			m = kmod_module_get_module(l);

			printf("modname: %s\n", kmod_module_get_name(m));
			kmod_module_unref(m);
		}
		kmod_module_unref_list(list);
	}

	kmod_unref(ctx);

	return EXIT_SUCCESS;
}
DEFINE_TEST(from_alias,
	.description = "check if aliases are parsed correctly",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-new-module/from_alias/",
	},
	.output = {
		.out = TESTSUITE_ROOTFS "test-new-module/from_alias/correct.txt",
	});

TESTSUITE_MAIN();
