// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2011-2013  ProFUSION embedded systems
 */

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libkmod/libkmod.h>

#include "kmod.h"

static int do_lsmod(int argc, char *argv[])
{
	struct kmod_ctx *ctx;
	const char *null_config = NULL;
	struct kmod_list *list, *itr;
	int err;

	if (argc != 1) {
		fprintf(stderr, "Usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}

	ctx = kmod_new(NULL, &null_config);
	if (ctx == NULL) {
		fputs("Error: kmod_new() failed!\n", stderr);
		return EXIT_FAILURE;
	}

	err = kmod_module_new_from_loaded(ctx, &list);
	if (err < 0) {
		fprintf(stderr, "Error: could not get list of modules: %s\n",
			strerror(-err));
		kmod_unref(ctx);
		return EXIT_FAILURE;
	}

	puts("Module                  Size  Used by");

	kmod_list_foreach(itr, list) {
		struct kmod_module *mod = kmod_module_get_module(itr);
		const char *name = kmod_module_get_name(mod);
		int use_count = kmod_module_get_refcnt(mod);
		long size = kmod_module_get_size(mod);
		struct kmod_list *holders = kmod_module_get_holders(mod), *hitr;
		int first = 1;

		if (holders == NULL || use_count < 0 || size < 0) {
			fprintf(stderr, "Error: could not query module %s! Was the module unloaded?\n", name);
			kmod_module_unref_list(holders);
			kmod_module_unref(mod);
			kmod_module_unref_list(list);
			kmod_unref(ctx);
			return EXIT_FAILURE;
		}

		printf("%-19s %8ld  %d", name, size, use_count);
		kmod_list_foreach(hitr, holders) {
			struct kmod_module *hm = kmod_module_get_module(hitr);

			if (!first) {
				putchar(',');
			} else {
				putchar(' ');
				first = 0;
			}

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

const struct kmod_cmd kmod_cmd_compat_lsmod = {
	.name = "lsmod",
	.cmd = do_lsmod,
	.help = "compat lsmod command",
};

const struct kmod_cmd kmod_cmd_list = {
	.name = "list",
	.cmd = do_lsmod,
	.help = "list currently loaded modules",
};
