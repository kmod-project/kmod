// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2011-2013  ProFUSION embedded systems
 */
#include <stdlib.h>
#include <string.h>

#include <shared/util.h>

#include <libkmod/libkmod.h>
#include "testsuite.h"

#define MAX_SOFTDEP_N 3

struct _test_softdep {
	const char *modname;
	const char *pre[MAX_SOFTDEP_N];
	const char *post[MAX_SOFTDEP_N];
};

struct _test_softdep test_modules[] = { { .modname = "mod-softdep-a",
					  .pre = { "mod_foo_a", "mod_foo_b", NULL },
					  .post = { "mod_foo_c", NULL } },
					{ .modname = "mod-softdep-b",
					  .pre = { "mod_foo_a", NULL },
					  .post = { "mod_foo_b", "mod_foo_c", NULL } } };

#define TEST_MODULE_SIZE ((int)(sizeof(test_modules) / sizeof(struct _test_softdep)))

static int check_dependencies(const char *modnames[], struct kmod_list *mod_list)
{
	struct kmod_list *itr;
	int visited[MAX_SOFTDEP_N] = { 0 };
	int mod_index;

	kmod_list_foreach(itr, mod_list) {
		struct kmod_module *softdep_mod = kmod_module_get_module(itr);
		const char *softdep_name = kmod_module_get_name(softdep_mod);
		printf(" %s", softdep_name);

		for (mod_index = 0; mod_index < MAX_SOFTDEP_N; mod_index++) {
			if (!modnames[mod_index])
				break;
			if (streq(softdep_name, modnames[mod_index]))
				visited[mod_index] = 1;
		}
		kmod_module_unref(softdep_mod);
	}
	printf("\n");

	for (mod_index = 0; mod_index < MAX_SOFTDEP_N; mod_index++) {
		if (!modnames[mod_index])
			break;
		if (!visited[mod_index]) {
			ERR("softdep %s not loaded\n", modnames[mod_index]);
			return -1;
		}
	}
	return 0;
}

static int softdep_mult(const struct test *t)
{
	struct kmod_ctx *ctx = NULL;
	struct kmod_module *mod = NULL;
	struct kmod_list *pre = NULL, *post = NULL;

	const char *modname;
	int mod_index;
	int err;

	ctx = kmod_new(NULL, NULL);
	if (ctx == NULL)
		exit(EXIT_FAILURE);

	for (mod_index = 0; mod_index < TEST_MODULE_SIZE; mod_index++) {
		modname = test_modules[mod_index].modname;
		printf("module %s:\n", modname);
		err = kmod_module_new_from_name(ctx, modname, &mod);
		if (err < 0)
			goto fail;
		pre = NULL;
		post = NULL;
		err = kmod_module_get_softdeps(mod, &pre, &post);
		if (err < 0) {
			ERR("could not get softdeps of '%s': %s\n", modname,
			    strerror(-err));
			goto fail;
		}

		printf("pre: ");
		err = check_dependencies(test_modules[mod_index].pre, pre);
		if (err < 0)
			goto fail;
		printf("post: ");
		err = check_dependencies(test_modules[mod_index].post, post);
		if (err < 0)
			goto fail;

		kmod_module_unref_list(pre);
		kmod_module_unref_list(post);
		kmod_module_unref(mod);
	}
	kmod_unref(ctx);
	return EXIT_SUCCESS;

fail:
	kmod_module_unref_list(pre);
	kmod_module_unref_list(post);
	kmod_module_unref(mod);
	kmod_unref(ctx);
	return EXIT_FAILURE;
}

/*
 * The current test case is expected to fail since the libkmod will take only
 * the first softdep statement.
 *
 * Remove the expected_fail option when the softdep search logic is updated.
 */
DEFINE_TEST(softdep_mult, .description = "check if multiple softdep is supported",
	    .config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-softdep-mult/",
	    }, .expected_fail=true);

TESTSUITE_MAIN();
