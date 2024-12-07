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

static int loaded_1(const struct test *t)
{
	struct kmod_ctx *ctx;
	const char *null_config = NULL;
	struct kmod_list *list, *itr;
	int err;

	ctx = kmod_new(NULL, &null_config);
	if (ctx == NULL)
		exit(EXIT_FAILURE);

	err = kmod_module_new_from_loaded(ctx, &list);
	if (err < 0) {
		fprintf(stderr, "%s\n", strerror(-err));
		kmod_unref(ctx);
		exit(EXIT_FAILURE);
	}

	printf("Module                  Size  Used by\n");

	kmod_list_foreach(itr, list) {
		struct kmod_module *mod = kmod_module_get_module(itr);
		const char *name = kmod_module_get_name(mod);
		int use_count = kmod_module_get_refcnt(mod);
		long size = kmod_module_get_size(mod);
		struct kmod_list *holders, *hitr;
		int first = 1;

		printf("%-19s %8ld  %d ", name, size, use_count);
		holders = kmod_module_get_holders(mod);
		kmod_list_foreach(hitr, holders) {
			struct kmod_module *hm = kmod_module_get_module(hitr);

			if (!first)
				putchar(',');
			else
				first = 0;

			fputs(kmod_module_get_name(hm), stdout);
			kmod_module_unref(hm);
		}
		putchar('\n');
		kmod_module_unref_list(holders);
		kmod_module_unref(mod);
	}
	kmod_module_unref_list(list);

	kmod_unref(ctx);

	return EXIT_SUCCESS;
}
DEFINE_TEST(loaded_1,
	.description = "check if list of module is created",
	.config = {
		[TC_ROOTFS] = "test-loaded/",
	},
	.output = {
		.out = "test-loaded/correct.txt",
	});

TESTSUITE_MAIN();
