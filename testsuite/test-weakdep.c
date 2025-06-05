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

#define EXEC_MODPROBE(...)                     \
	test_spawn_prog(TOOLS_DIR "/modprobe", \
			(const char *[]){ TOOLS_DIR "/modprobe", ##__VA_ARGS__, NULL })

static const char *const mod_name[] = {
	"mod-loop-b",
	"mod-weakdep",
	NULL,
};

static int test_weakdep(const struct test *t)
{
	struct kmod_ctx *ctx;
	int mod_name_index = 0;
	int err;

	ctx = kmod_new(NULL, NULL);
	if (ctx == NULL)
		exit(EXIT_FAILURE);

	while (mod_name[mod_name_index]) {
		struct kmod_list *list = NULL;
		struct kmod_module *mod = NULL;
		struct kmod_list *mod_list = NULL;
		struct kmod_list *itr = NULL;

		printf("%s:", mod_name[mod_name_index]);
		err = kmod_module_new_from_lookup(ctx, mod_name[mod_name_index], &list);
		if (list == NULL || err < 0) {
			fprintf(stderr, "module %s not found in directory %s\n",
				mod_name[mod_name_index],
				ctx ? kmod_get_dirname(ctx) : "(missing)");
			exit(EXIT_FAILURE);
		}

		mod = kmod_module_get_module(list);

		err = kmod_module_get_weakdeps(mod, &mod_list);
		if (err) {
			fprintf(stderr, "weak dependencies can not be read for %s (%d)\n",
				mod_name[mod_name_index], err);
			exit(EXIT_FAILURE);
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

		mod_name_index++;
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

static noreturn int modprobe_config(const struct test *t)
{
	EXEC_MODPROBE("-c");
	exit(EXIT_FAILURE);
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
