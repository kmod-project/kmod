// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright 2024 Red Hat
 * Copyright (C) 2011-2013  ProFUSION embedded systems
 * Copyright (C) 2015  Intel Corporation. All rights reserved.
 * Copyright © 2025 Intel Corporation
 */

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>

#include <shared/macro.h>
#include <shared/util.h>

#include <libkmod/libkmod.h>

/* good luck building a kmod_list outside of the library... makes the blacklist
 * function rather pointless */
/* FIXME: convert the test so we don't need the internal header */
#include <libkmod/libkmod-internal.h>

#include "testsuite.h"

static int test_load_resources(void)
{
	struct kmod_ctx *ctx;
	const char *null_config = NULL;
	int err;

	ctx = kmod_new(NULL, &null_config);
	TS_ASSERT(ctx != NULL);

	kmod_set_log_priority(ctx, 7);

	err = kmod_load_resources(ctx);
	TS_ASSERT(err == 0);

	kmod_unref(ctx);

	return 0;
}
DEFINE_TEST_WITH_FUNC(
	test_load_resource1, test_load_resources,
	.description =
		"test if kmod_load_resources works (recent modprobe on kernel without modules.builtin.modinfo)",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-init-load-resources/",
		[TC_UNAME_R] = "5.6.0",
	});

DEFINE_TEST_WITH_FUNC(
	test_load_resource2, test_load_resources,
	.description =
		"test if kmod_load_resources works with empty modules.builtin.aliases.bin (recent depmod on kernel without modules.builtin.modinfo)",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS
		"test-init-load-resources-empty-builtin-aliases-bin/",
		[TC_UNAME_R] = "5.6.0",
	});

static int test_initlib(void)
{
	struct kmod_ctx *ctx;
	const char *null_config = NULL;

	ctx = kmod_new(NULL, &null_config);
	TS_ASSERT(ctx != NULL);

	kmod_unref(ctx);

	return 0;
}
DEFINE_TEST(test_initlib, .description = "test if libkmod's init function work");

static int test_insert(void)
{
	struct kmod_ctx *ctx;
	struct kmod_module *mod;
	const char *null_config = NULL;
	int err;

	ctx = kmod_new(NULL, &null_config);
	TS_ASSERT(ctx != NULL);

	err = kmod_module_new_from_path(ctx, "/mod-simple.ko", &mod);
	TS_ASSERT(err == 0);

	err = kmod_module_insert_module(mod, 0, NULL);
	TS_ASSERT(err == 0);

	kmod_module_unref(mod);
	kmod_unref(ctx);

	return 0;
}
DEFINE_TEST(test_insert,
	.description = "test if libkmod's insert_module returns ok",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-init/",
		[TC_INIT_MODULE_RETCODES] = "bla:1:20",
	},
	.modules_loaded = "mod_simple");

static int test_remove(void)
{
	struct kmod_ctx *ctx;
	struct kmod_module *mod_simple, *mod_bla;
	const char *null_config = NULL;
	int err;

	ctx = kmod_new(NULL, &null_config);
	TS_ASSERT(ctx != NULL);

	err = kmod_module_new_from_name(ctx, "mod-simple", &mod_simple);
	TS_ASSERT(err == 0);

	err = kmod_module_new_from_name(ctx, "bla", &mod_bla);
	TS_ASSERT(err == 0);

	err = kmod_module_remove_module(mod_simple, 0);
	TS_ASSERT(err == 0);

	err = kmod_module_remove_module(mod_bla, 0);
	TS_ASSERT(err == -ENOENT);

	kmod_module_unref(mod_bla);
	kmod_module_unref(mod_simple);
	kmod_unref(ctx);

	return 0;
}
DEFINE_TEST(
	test_remove, .description = "test if libkmod's remove_module returns ok",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-remove/",
		[TC_DELETE_MODULE_RETCODES] = "mod-simple:0:0:bla:-1:" STRINGIFY(ENOENT),
	});

static int test_remove2(void)
{
	struct kmod_ctx *ctx;
	struct kmod_module *mod;
	const char *null_config = NULL;
	int err;
	struct stat st;

	ctx = kmod_new(NULL, &null_config);
	TS_ASSERT(ctx != NULL);

	err = kmod_module_new_from_path(ctx, "/mod-simple.ko", &mod);
	TS_ASSERT(err == 0);

	err = kmod_module_insert_module(mod, 0, NULL);
	TS_ASSERT(err == 0);

	err = kmod_module_remove_module(mod, 0);
	TS_ASSERT(err == 0);

	TS_ASSERT(stat("/sys/module/mod_simple", &st) != 0 || !S_ISDIR(st.st_mode));

	kmod_module_unref(mod);
	kmod_unref(ctx);

	return 0;
}
DEFINE_TEST(test_remove2,
	    .description = "test if libkmod's delete_module removes module directory",
	    .config = {
		    [TC_ROOTFS] = TESTSUITE_ROOTFS "test-remove2/",
		    [TC_INIT_MODULE_RETCODES] = "",
		    [TC_DELETE_MODULE_RETCODES] = "mod_simple:0:0" STRINGIFY(ENOENT),
	    });

static int from_name(void)
{
	static const char *const modnames[] = {
		// clang-format off
		"ext4",
		"balbalbalbbalbalbalbalbalbalbal",
		"snd-hda-intel",
		"snd-timer",
		"iTCO_wdt",
		// clang-format on
	};
	struct kmod_ctx *ctx;
	struct kmod_module *mod;
	const char *null_config = NULL;
	int err;

	ctx = kmod_new(NULL, &null_config);
	TS_ASSERT(ctx != NULL);

	for (size_t i = 0; i < ARRAY_SIZE(modnames); i++) {
		err = kmod_module_new_from_name(ctx, modnames[i], &mod);
		TS_ASSERT(err == 0);

		printf("modname: %s\n", kmod_module_get_name(mod));
		kmod_module_unref(mod);
	}

	kmod_unref(ctx);

	return 0;
}
DEFINE_TEST(from_name,
	.description = "check if module names are parsed correctly",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-new-module/from_name/",
	},
	.output = {
		.out = TESTSUITE_ROOTFS "test-new-module/from_name/correct.txt",
	});

static int from_alias(void)
{
	static const char *const modnames[] = {
		"ext4.*",
	};
	struct kmod_ctx *ctx;
	int err;

	ctx = kmod_new(NULL, NULL);
	TS_ASSERT(ctx != NULL);

	for (size_t i = 0; i < ARRAY_SIZE(modnames); i++) {
		struct kmod_list *l, *list = NULL;

		err = kmod_module_new_from_lookup(ctx, modnames[i], &list);
		TS_ASSERT(err == 0);

		kmod_list_foreach(l, list) {
			struct kmod_module *m;
			m = kmod_module_get_module(l);

			printf("modname: %s\n", kmod_module_get_name(m));
			kmod_module_unref(m);
		}
		kmod_module_unref_list(list);
	}

	kmod_unref(ctx);

	return 0;
}
DEFINE_TEST(from_alias,
	.description = "check if aliases are parsed correctly",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-new-module/from_alias/",
	},
	.output = {
		.out = TESTSUITE_ROOTFS "test-new-module/from_alias/correct.txt",
	});

static int test_initstate_from_lookup(void)
{
	struct kmod_ctx *ctx;
	struct kmod_list *list = NULL;
	struct kmod_module *mod;
	const char *null_config = NULL;
	int err, r;

	ctx = kmod_new(NULL, &null_config);
	TS_ASSERT(ctx != NULL);

	err = kmod_module_new_from_lookup(ctx, "fake-builtin", &list);
	TS_ASSERT(err == 0);
	TS_ASSERT(list != NULL);

	mod = kmod_module_get_module(list);

	r = kmod_module_get_initstate(mod);
	TS_ASSERT(r == KMOD_MODULE_BUILTIN);

	kmod_module_unref(mod);
	kmod_module_unref_list(list);
	kmod_unref(ctx);

	return 0;
}
DEFINE_TEST(
	test_initstate_from_lookup,
	.description =
		"test if libkmod return correct initstate for builtin module from lookup",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-initstate",
		[TC_UNAME_R] = "4.4.4",
	});

static int test_initstate_from_name(void)
{
	struct kmod_ctx *ctx;
	struct kmod_module *mod = NULL;
	const char *null_config = NULL;
	int err, r;

	ctx = kmod_new(NULL, &null_config);
	TS_ASSERT(ctx != NULL);

	err = kmod_module_new_from_name(ctx, "fake-builtin", &mod);
	TS_ASSERT(err == 0);
	TS_ASSERT(mod != NULL);

	r = kmod_module_get_initstate(mod);
	TS_ASSERT(r == KMOD_MODULE_BUILTIN);

	kmod_module_unref(mod);
	kmod_unref(ctx);

	return 0;
}
DEFINE_TEST(test_initstate_from_name,
	    .description =
		    "test if libkmod return correct initstate for builtin module from name",
	    .config = {
		    [TC_ROOTFS] = TESTSUITE_ROOTFS "test-initstate",
		    [TC_UNAME_R] = "4.4.4",
	    });

static int blacklist_1(void)
{
	struct kmod_ctx *ctx;
	struct kmod_list *list = NULL, *l, *filtered;
	struct kmod_module *mod;
	int err;
	size_t len = 0;

	static const char *const names[] = { "pcspkr", "pcspkr2", "floppy", "ext4" };

	ctx = kmod_new(NULL, NULL);
	TS_ASSERT(ctx != NULL);

	for (size_t i = 0; i < ARRAY_SIZE(names); i++) {
		err = kmod_module_new_from_name(ctx, names[i], &mod);
		TS_ASSERT(err == 0);
		list = kmod_list_append(list, mod);
	}

	err = kmod_module_apply_filter(ctx, KMOD_FILTER_BLACKLIST, list, &filtered);
	TS_ASSERT(err == 0);
	TS_ASSERT(filtered != NULL);

	kmod_list_foreach(l, filtered) {
		const char *modname;
		mod = kmod_module_get_module(l);
		modname = kmod_module_get_name(mod);
		TS_ASSERT(!streq("pcspkr", modname) && !streq("floppy", modname));
		len++;
		kmod_module_unref(mod);
	}

	TS_ASSERT(len == 2);

	kmod_module_unref_list(filtered);
	kmod_module_unref_list(list);
	kmod_unref(ctx);

	return 0;
}

DEFINE_TEST(blacklist_1, .description = "check if modules are correctly blacklisted",
	    .config = {
		    [TC_ROOTFS] = TESTSUITE_ROOTFS "test-blacklist/",
	    });

static int loaded_1(void)
{
	struct kmod_ctx *ctx;
	const char *null_config = NULL;
	struct kmod_list *list, *itr;
	int err;

	ctx = kmod_new(NULL, &null_config);
	TS_ASSERT(ctx != NULL);

	err = kmod_module_new_from_loaded(ctx, &list);
	TS_ASSERT(err == 0);

	printf("Module                  Size  Used by\n");

	kmod_list_foreach(itr, list) {
		struct kmod_module *mod = kmod_module_get_module(itr);
		const char *name = kmod_module_get_name(mod);
		int use_count = kmod_module_get_refcnt(mod);
		long size = kmod_module_get_size(mod);
		struct kmod_list *holders, *hitr;
		int first = 1;

		printf("%-19s %8ld  %d ", name, size, use_count);
		holders = kmod_module_get_holders(mod);
		kmod_list_foreach(hitr, holders) {
			struct kmod_module *hm = kmod_module_get_module(hitr);

			if (!first)
				putchar(',');
			else
				first = 0;

			fputs(kmod_module_get_name(hm), stdout);
			kmod_module_unref(hm);
		}
		putchar('\n');
		kmod_module_unref_list(holders);
		kmod_module_unref(mod);
	}
	kmod_module_unref_list(list);

	kmod_unref(ctx);

	return 0;
}
DEFINE_TEST(loaded_1,
	.description = "check if list of module is created",
	.config = {
		[TC_ROOTFS] = TESTSUITE_ROOTFS "test-loaded/",
	},
	.output = {
		.out = TESTSUITE_ROOTFS "test-loaded/correct.txt",
	});

#define TEST_UNAME "4.0.20-kmod"

static int test_dependencies(void)
{
	struct kmod_ctx *ctx;
	struct kmod_module *mod = NULL;
	struct kmod_list *list, *l;
	int err;
	size_t len = 0;
	int fooa = 0, foob = 0, fooc = 0;

	ctx = kmod_new(NULL, NULL);
	TS_ASSERT(ctx != NULL);

	err = kmod_module_new_from_name(ctx, "mod-foo", &mod);
	TS_ASSERT(err == 0 && mod != NULL);

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
	TS_ASSERT(len == 3 && fooa && foob && fooc);

	kmod_module_unref_list(list);
	kmod_module_unref(mod);
	kmod_unref(ctx);

	return 0;
}
DEFINE_TEST(test_dependencies,
	    .description = "test if kmod_module_get_dependencies works",
	    .config = {
		    [TC_UNAME_R] = TEST_UNAME,
		    [TC_ROOTFS] = TESTSUITE_ROOTFS "test-dependencies/",
	    });

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
	return 0;
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

static int test_weakdep(void)
{
	static const char *const mod_name[] = {
		"mod-loop-b",
		"mod-weakdep",
	};
	struct kmod_ctx *ctx;
	int err;

	ctx = kmod_new(NULL, NULL);
	TS_ASSERT(ctx != NULL);

	for (size_t i = 0; i < ARRAY_SIZE(mod_name); i++) {
		struct kmod_list *list = NULL;
		struct kmod_module *mod = NULL;
		struct kmod_list *mod_list = NULL;
		struct kmod_list *itr = NULL;

		printf("%s:", mod_name[i]);
		err = kmod_module_new_from_lookup(ctx, mod_name[i], &list);
		TS_ASSERT(list != NULL && err == 0);

		mod = kmod_module_get_module(list);

		err = kmod_module_get_weakdeps(mod, &mod_list);
		TS_ASSERT(err == 0);

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

	return 0;
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

TESTSUITE_MAIN();
