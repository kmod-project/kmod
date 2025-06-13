// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright Red Hat
 */

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libkmod/libkmod.h>

#include "testsuite.h"

static int test_weakdep(void)
{
	static const char *const mod_name[] = {
		"mod-loop-b",
		"mod-weakdep",
	};
	struct kmod_ctx *ctx;
	int err;

	ctx = kmod_new(NULL, NULL);
	if (ctx == NULL)
		return EXIT_FAILURE;

	for (size_t i = 0; i < ARRAY_SIZE(mod_name); i++) {
		struct kmod_list *list = NULL;
		struct kmod_module *mod = NULL;
		struct kmod_list *mod_list = NULL;
		struct kmod_list *itr = NULL;

		printf("%s:", mod_name[i]);
		err = kmod_module_new_from_lookup(ctx, mod_name[i], &list);
		if (list == NULL || err < 0) {
			fprintf(stderr, "module %s not found in directory %s\n",
				mod_name[i], ctx ? kmod_get_dirname(ctx) : "(missing)");
			return EXIT_FAILURE;
		}

		mod = kmod_module_get_module(list);

		err = kmod_module_get_weakdeps(mod, &mod_list);
		if (err) {
			fprintf(stderr, "weak dependencies can not be read for %s (%d)\n",
				mod_name[i], err);
			return EXIT_FAILURE;
		}

		kmod_list_foreach(itr, mod_list) {
			struct kmod_module *weakdep_mod = kmod_module_get_module(itr);
			const char *weakdep_name = kmod_module_get_name(weakdep_mod);

			printf(" %s", weakdep_name);
			kmod_module_unref(weakdep_mod);
		}
		printf("\n");

		kmod_module_unref_list(mod_list);
		kmod_module_unref(mod);
		kmod_module_unref_list(list);
	}

	kmod_unref(ctx);

	return EXIT_SUCCESS;
}
DEFINE_TEST(test_weakdep,
	.description = "check if libkmod breaks weakdep",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-weakdep",
		[TC_INIT_MODULE_RETCODES] = "",
	},
	.output = {
		.out = TESTSUITE_ROOTFS "test-weakdep/correct-weakdep.txt",
	});

static int modprobe_config(void)
{
	return EXEC_TOOL(modprobe, "-c");
}
DEFINE_TEST(modprobe_config,
	.description = "check modprobe config parsing with weakdep",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-weakdep",
	},
	.output = {
		.out = TESTSUITE_ROOTFS "test-weakdep/modprobe-c.txt",
	});

TESTSUITE_MAIN();
