/*
 * Copyright (C) 2012  ProFUSION embedded systems
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <libkmod.h>

#include "testsuite.h"

static int from_name(const struct test *t)
{
	static const char *modnames[] = {
		"ext4",
		"balbalbalbbalbalbalbalbalbalbal",
		"snd-hda-intel",
		"snd-timer",
		"iTCO_wdt",
		NULL,
	};
	const char **p;
	struct kmod_ctx *ctx;
	struct kmod_module *mod;
	const char *null_config = NULL;
	int err;

	ctx = kmod_new(NULL, &null_config);
	if (ctx == NULL)
		exit(EXIT_FAILURE);

	for (p = modnames; p != NULL; p++) {
		err = kmod_module_new_from_name(ctx, *p, &mod);
		if (err < 0)
			exit(EXIT_SUCCESS);

		printf("modname: %s\n", kmod_module_get_name(mod));
		kmod_module_unref(mod);
	}

	kmod_unref(ctx);

	return EXIT_SUCCESS;
}
static const struct test sfrom_name = {
	.name = "sfrom_name",
	.description = "check if module names are parsed correctly",
	.func = from_name,
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-new-module/from_name/",
	},
	.need_spawn = true,
	.output = {
		.stdout = TESTSUITE_ROOTFS "test-new-module/from_name/correct.txt",
	},
};


static int from_alias(const struct test *t)
{
	static const char *modnames[] = {
		"ext4.*",
		NULL,
	};
	const char **p;
	struct kmod_ctx *ctx;
	int err;

	ctx = kmod_new(NULL, NULL);
	if (ctx == NULL)
		exit(EXIT_FAILURE);

	for (p = modnames; p != NULL; p++) {
		struct kmod_list *l, *list = NULL;

		err = kmod_module_new_from_lookup(ctx, *p, &list);
		if (err < 0)
			exit(EXIT_SUCCESS);

		kmod_list_foreach(l, list) {
			struct kmod_module *m;
			m = kmod_module_get_module(l);

			printf("modname: %s\n", kmod_module_get_name(m));
			kmod_module_unref(m);
		}
		kmod_module_unref_list(list);
	}

	kmod_unref(ctx);

	return EXIT_SUCCESS;
}
static const struct test sfrom_alias = {
	.name = "sfrom_alias",
	.description = "check if aliases are parsed correctly",
	.func = from_alias,
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-new-module/from_alias/",
	},
	.need_spawn = true,
	.output = {
		.stdout = TESTSUITE_ROOTFS "test-new-module/from_alias/correct.txt",
	},
};

static const struct test *tests[] = {
	&sfrom_name,
	&sfrom_alias,
	NULL,
};

int main(int argc, char *argv[])
{
	const struct test *t;
	int arg;
	size_t i;

	arg = test_init(argc, argv, tests);
	if (arg == 0)
		return 0;

	if (arg < argc) {
		t = test_find(tests, argv[arg]);
		if (t == NULL) {
			fprintf(stderr, "could not find test %s\n", argv[arg]);
			exit(EXIT_FAILURE);
		}

		return test_run(t);
	}

	for (i = 0; tests[i] != NULL; i++) {
		if (test_run(tests[i]) != 0)
			exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}
