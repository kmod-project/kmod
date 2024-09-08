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

#include <libkmod/libkmod.h>

#include "testsuite.h"

static int from_name(const struct test *t)
{
	static const char *const modnames[] = {
		// clang-format off
		"ext4",
		"balbalbalbbalbalbalbalbalbalbal",
		"snd-hda-intel",
		"snd-timer",
		"iTCO_wdt",
		NULL,
		// clang-format on
	};
	const char *const *p;
	struct kmod_ctx *ctx;
	struct kmod_module *mod;
	const char *null_config = NULL;
	int err;

	ctx = kmod_new(NULL, &null_config);
	if (ctx == NULL)
		exit(EXIT_FAILURE);

	for (p = modnames; p != NULL; p++) {
		err = kmod_module_new_from_name(ctx, *p, &mod);
		if (err < 0)
			exit(EXIT_SUCCESS);

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
	.need_spawn = true,
	.output = {
		.out = TESTSUITE_ROOTFS "test-new-module/from_name/correct.txt",
	});

static int from_alias(const struct test *t)
{
	static const char *const modnames[] = {
		"ext4.*",
		NULL,
	};
	const char *const *p;
	struct kmod_ctx *ctx;
	int err;

	ctx = kmod_new(NULL, NULL);
	if (ctx == NULL)
		exit(EXIT_FAILURE);

	for (p = modnames; p != NULL; p++) {
		struct kmod_list *l, *list = NULL;

		err = kmod_module_new_from_lookup(ctx, *p, &list);
		if (err < 0)
			exit(EXIT_SUCCESS);

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
	.need_spawn = true,
	.output = {
		.out = TESTSUITE_ROOTFS "test-new-module/from_alias/correct.txt",
	});

TESTSUITE_MAIN();
