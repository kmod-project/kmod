// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * kmod-rmmod - remove modules from linux kernel using libkmod.
 *
 * Copyright (C) 2011-2013  ProFUSION embedded systems
 */

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <shared/macro.h>

#include <libkmod/libkmod.h>

#include "kmod.h"

static const char cmdopts_s[] = "fsvVh";
static const struct option cmdopts[] = {
	// clang-format off
	{ "force", no_argument, 0, 'f' },
	{ "syslog", no_argument, 0, 's' },
	{ "verbose", no_argument, 0, 'v' },
	{ "version", no_argument, 0, 'V' },
	{ "help", no_argument, 0, 'h' },
	{ NULL, 0, 0, 0 },
	// clang-format on
};

static void help(void)
{
	printf("Usage:\n"
	       "\t%s [options] [list of modulenames]\n"
	       "Options:\n"
	       "\t-f, --force       DANGEROUS: forces a module unload and may\n"
	       "\t                  crash your machine\n"
	       "\t-s, --syslog      print to syslog, not stderr\n"
	       "\t-v, --verbose     enables more messages\n"
	       "\t-V, --version     show version\n"
	       "\t-h, --help        show this help\n",
	       program_invocation_short_name);
}

static int check_module_inuse(struct kmod_module *mod)
{
	struct kmod_list *holders;
	int state, ret;

	state = kmod_module_get_initstate(mod);

	if (state == KMOD_MODULE_BUILTIN) {
		ERR("Module %s is builtin.\n", kmod_module_get_name(mod));
		return -ENOENT;
	} else if (state < 0) {
		ERR("Module %s is not currently loaded\n", kmod_module_get_name(mod));
		return -ENOENT;
	}

	holders = kmod_module_get_holders(mod);
	if (holders != NULL) {
		struct kmod_list *itr;

		ERR("Module %s is in use by:", kmod_module_get_name(mod));

		kmod_list_foreach(itr, holders) {
			struct kmod_module *hm = kmod_module_get_module(itr);
			ERR(" %s", kmod_module_get_name(hm));
			kmod_module_unref(hm);
		}
		ERR("\n");

		kmod_module_unref_list(holders);
		return -EBUSY;
	}

	ret = kmod_module_get_refcnt(mod);
	if (ret > 0) {
		ERR("Module %s is in use\n", kmod_module_get_name(mod));
		return -EBUSY;
	} else if (ret == -ENOENT) {
		ERR("Module unloading is not supported\n");
	}

	return ret;
}

static int do_rmmod(int argc, char *argv[])
{
	struct kmod_ctx *ctx;
	const char *null_config = NULL;
	int verbose = LOG_ERR;
	int use_syslog = 0;
	int flags = 0;
	int i, c, r = 0;

	while ((c = getopt_long(argc, argv, cmdopts_s, cmdopts, NULL)) != -1) {
		switch (c) {
		case 'f':
			flags |= KMOD_REMOVE_FORCE;
			break;
		case 's':
			use_syslog = 1;
			break;
		case 'v':
			verbose++;
			break;
		case 'h':
			help();
			return EXIT_SUCCESS;
		case 'V':
			kmod_version();
			return EXIT_SUCCESS;
		case '?':
			return EXIT_FAILURE;
		default:
			ERR("unexpected getopt_long() value '%c'.\n", c);
			return EXIT_FAILURE;
		}
	}

	log_open(use_syslog);

	if (optind >= argc) {
		ERR("missing module name.\n");
		r = EXIT_FAILURE;
		goto done;
	}

	ctx = kmod_new(NULL, &null_config);
	if (!ctx) {
		ERR("kmod_new() failed!\n");
		r = EXIT_FAILURE;
		goto done;
	}

	log_setup_kmod_log(ctx, verbose);

	for (i = optind; i < argc; i++) {
		struct kmod_module *mod;
		const char *arg = argv[i];
		struct stat st;
		int err;

		if (stat(arg, &st) == 0)
			err = kmod_module_new_from_path(ctx, arg, &mod);
		else
			err = kmod_module_new_from_name(ctx, arg, &mod);

		if (err < 0) {
			ERR("could not use module %s: %s\n", arg, strerror(-err));
			r = EXIT_FAILURE;
			break;
		}

		if (!(flags & KMOD_REMOVE_FORCE) && check_module_inuse(mod) < 0) {
			r = EXIT_FAILURE;
			kmod_module_unref(mod);
			continue;
		}

		err = kmod_module_remove_module(mod, flags);
		if (err < 0) {
			ERR("could not remove module %s: %s\n", arg, strerror(-err));
			r = EXIT_FAILURE;
		}

		kmod_module_unref(mod);
	}

	kmod_unref(ctx);

done:
	log_close();

	return r == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

const struct kmod_cmd kmod_cmd_compat_rmmod = {
	.name = "rmmod",
	.cmd = do_rmmod,
	.help = "compat rmmod command",
};
