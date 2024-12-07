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

#include "testsuite.h"

#define TEST_UNAME "4.0.20-kmod"

static noreturn int test_dependencies(const struct test *t)
{
	struct kmod_ctx *ctx;
	struct kmod_module *mod = NULL;
	struct kmod_list *list, *l;
	int err;
	size_t len = 0;
	int fooa = 0, foob = 0, fooc = 0;

	ctx = kmod_new(NULL, NULL);
	if (ctx == NULL)
		exit(EXIT_FAILURE);

	err = kmod_module_new_from_name(ctx, "mod-foo", &mod);
	if (err < 0 || mod == NULL) {
		kmod_unref(ctx);
		exit(EXIT_FAILURE);
	}

	list = kmod_module_get_dependencies(mod);

	kmod_list_foreach(l, list) {
		struct kmod_module *m = kmod_module_get_module(l);
		const char *name = kmod_module_get_name(m);

		if (streq(name, "mod_foo_a"))
			fooa = 1;
		if (streq(name, "mod_foo_b"))
			foob = 1;
		else if (streq(name, "mod_foo_c"))
			fooc = 1;

		fprintf(stderr, "name=%s", name);
		kmod_module_unref(m);
		len++;
	}

	/* fooa, foob, fooc */
	if (len != 3 || !fooa || !foob || !fooc)
		exit(EXIT_FAILURE);

	kmod_module_unref_list(list);
	kmod_module_unref(mod);
	kmod_unref(ctx);

	exit(EXIT_SUCCESS);
}
DEFINE_TEST(test_dependencies,
	    .description = "test if kmod_module_get_dependencies works",
	    .config = {
		    [TC_UNAME_R] = TEST_UNAME,
		    [TC_ROOTFS] = "test-dependencies/",
	    });

TESTSUITE_MAIN();
