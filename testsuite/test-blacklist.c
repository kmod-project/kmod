// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2011-2013  ProFUSION embedded systems
 */

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <shared/util.h>

#include <libkmod/libkmod.h>

/* good luck building a kmod_list outside of the library... makes this blacklist
 * function rather pointless */
#include <libkmod/libkmod-internal.h>

/* FIXME: hack, change name so we don't clash */
#undef ERR
#include "testsuite.h"

static int blacklist_1(void)
{
	struct kmod_ctx *ctx;
	struct kmod_list *list = NULL, *l, *filtered;
	struct kmod_module *mod;
	int err;
	size_t len = 0;

	static const char *const names[] = { "pcspkr", "pcspkr2", "floppy", "ext4" };

	ctx = kmod_new(NULL, NULL);
	OK(ctx != NULL, "Failed to create kmod context");

	for (size_t i = 0; i < ARRAY_SIZE(names); i++) {
		err = kmod_module_new_from_name(ctx, names[i], &mod);
		OK(err == 0, "Failed to lookup new module");
		list = kmod_list_append(list, mod);
	}

	err = kmod_module_apply_filter(ctx, KMOD_FILTER_BLACKLIST, list, &filtered);
	OK(err == 0, "Could not filter: %s", strerror(-err));
	OK(filtered != NULL, "All modules were filtered out!");

	kmod_list_foreach(l, filtered) {
		const char *modname;
		mod = kmod_module_get_module(l);
		modname = kmod_module_get_name(mod);
		OK(!streq("pcspkr", modname) && !streq("floppy", modname),
		   "Module should have been filtered out");
		len++;
		kmod_module_unref(mod);
	}

	OK(len == 2, "Invalid number of unfiltered modules");

	kmod_module_unref_list(filtered);
	kmod_module_unref_list(list);
	kmod_unref(ctx);

	return EXIT_SUCCESS;
}

DEFINE_TEST(blacklist_1, .description = "check if modules are correctly blacklisted",
	    .config = {
		    [TC_ROOTFS] = TESTSUITE_ROOTFS "test-blacklist/",
	    });

TESTSUITE_MAIN();
