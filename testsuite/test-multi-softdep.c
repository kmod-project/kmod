// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright © 2025 Intel Corporation
 */
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <shared/util.h>

#include <libkmod/libkmod.h>
#include "testsuite.h"

#define MAX_SOFTDEP_N 3

static const struct {
	const char *modname;
	const char *pre[MAX_SOFTDEP_N];
	const char *post[MAX_SOFTDEP_N];
} test_modules[] = {
	{
		.modname = "mod-softdep-a",
		.pre = { "mod_foo_a", "mod_foo_b" },
		.post = { "mod_foo_c" },
	},
	{
		.modname = "mod-softdep-b",
		.pre = { "mod_foo_a" },
		.post = { "mod_foo_b", "mod_foo_c" },
	},
};

static int check_dependencies(const char *const *modnames,
			      const struct kmod_list *mod_list)
{
	const struct kmod_list *itr;
	bool visited[MAX_SOFTDEP_N] = {};
	int mod_index;

	kmod_list_foreach(itr, mod_list) {
		struct kmod_module *softdep_mod = kmod_module_get_module(itr);
		const char *softdep_name = kmod_module_get_name(softdep_mod);
		printf(" %s", softdep_name);

		for (mod_index = 0; mod_index < MAX_SOFTDEP_N; mod_index++) {
			if (!modnames[mod_index])
				break;
			if (streq(softdep_name, modnames[mod_index]))
				visited[mod_index] = true;
		}
		kmod_module_unref(softdep_mod);
	}
	printf("\n");

	for (mod_index = 0; mod_index < MAX_SOFTDEP_N; mod_index++) {
		if (!modnames[mod_index])
			break;
		TS_ASSERT(visited[mod_index]);
	}
	return 0;
}

static int multi_softdep(void)
{
	struct kmod_ctx *ctx = NULL;
	struct kmod_module *mod = NULL;
	struct kmod_list *pre = NULL, *post = NULL;

	const char *modname;
	size_t mod_index;
	int err;

	ctx = kmod_new(NULL, NULL);
	TS_ASSERT(ctx != NULL);

	for (mod_index = 0; mod_index < ARRAY_SIZE(test_modules); mod_index++) {
		modname = test_modules[mod_index].modname;
		printf("module %s:\n", modname);
		err = kmod_module_new_from_name(ctx, modname, &mod);
		TS_ASSERT(err == 0);

		pre = NULL;
		post = NULL;
		err = kmod_module_get_softdeps(mod, &pre, &post);
		TS_ASSERT(err == 0);

		printf("pre: ");
		err = check_dependencies(test_modules[mod_index].pre, pre);
		TS_ASSERT(err == 0);

		printf("post: ");
		err = check_dependencies(test_modules[mod_index].post, post);
		TS_ASSERT(err == 0);

		kmod_module_unref_list(pre);
		kmod_module_unref_list(post);
		kmod_module_unref(mod);
	}
	kmod_unref(ctx);
	return EXIT_SUCCESS;
}

/*
 * FIXME: This test indicates a bug in kmod. Once fixed, the expected_fail should be removed.
 * See https://github.com/kmod-project/kmod/issues/33 for more details.
 */
DEFINE_TEST(multi_softdep, .description = "check if multiple softdep is supported",
	    .expected_fail = true,
	    .config = {
		    [TC_UNAME_R] = "4.4.4",
		    [TC_ROOTFS] = TESTSUITE_ROOTFS "test-multi-softdep/",
	    });

TESTSUITE_MAIN();
