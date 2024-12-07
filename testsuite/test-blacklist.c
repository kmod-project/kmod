// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2011-2013  ProFUSION embedded systems
 */

#include <errno.h>
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

static int blacklist_1(const struct test *t)
{
	struct kmod_ctx *ctx;
	struct kmod_list *list = NULL, *l, *filtered;
	struct kmod_module *mod;
	int err;
	size_t len = 0;

	const char *names[] = { "pcspkr", "pcspkr2", "floppy", "ext4", NULL };
	const char **name;

	ctx = kmod_new(NULL, NULL);
	if (ctx == NULL)
		exit(EXIT_FAILURE);

	for (name = names; *name; name++) {
		err = kmod_module_new_from_name(ctx, *name, &mod);
		if (err < 0)
			goto fail_lookup;
		list = kmod_list_append(list, mod);
	}

	err = kmod_module_apply_filter(ctx, KMOD_FILTER_BLACKLIST, list, &filtered);
	if (err < 0) {
		ERR("Could not filter: %s\n", strerror(-err));
		goto fail;
	}
	if (filtered == NULL) {
		ERR("All modules were filtered out!\n");
		goto fail;
	}

	kmod_list_foreach(l, filtered) {
		const char *modname;
		mod = kmod_module_get_module(l);
		modname = kmod_module_get_name(mod);
		if (streq("pcspkr", modname) || streq("floppy", modname))
			goto fail;
		len++;
		kmod_module_unref(mod);
	}

	if (len != 2)
		goto fail;

	kmod_module_unref_list(filtered);
	kmod_module_unref_list(list);
	kmod_unref(ctx);

	return EXIT_SUCCESS;

fail:
	kmod_module_unref_list(list);
fail_lookup:
	kmod_unref(ctx);
	return EXIT_FAILURE;
}

DEFINE_TEST(blacklist_1, .description = "check if modules are correctly blacklisted",
	    .config = {
		    [TC_ROOTFS] = "test-blacklist/",
	    });

TESTSUITE_MAIN();
