/*
 * Copyright Red Hat
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
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

#define TEST_USER_ROOTFS TESTSUITE_ROOTFS "test-user/"
#define TEST_USER_KERNEL_DIR TEST_USER_ROOTFS "lib/modules/4.4.4/"

static const char *const test_user_config_paths[] = {
	TEST_USER_ROOTFS "etc/modprobe.d",
	NULL
};

static const char *const mod_name[] = {
	"mod-loop-b",
	"mod-weakdep",
	NULL
};

static int test_user_weakdep(const struct test *t)
{
	struct kmod_ctx *ctx;
	int mod_name_index = 0;
	int err;

	ctx = kmod_new(TEST_USER_KERNEL_DIR, test_user_config_paths);
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
DEFINE_TEST(test_user_weakdep,
	.description = "check if modprobe breaks weakdep2",
	.config = {
		[TC_UNAME_R] = "4.4.4",
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-user",
		[TC_INIT_MODULE_RETCODES] = "",
	},
	.need_spawn = true,
	.output = {
		.out = TESTSUITE_ROOTFS "test-user/correct-weakdep.txt",
	});

TESTSUITE_MAIN();
