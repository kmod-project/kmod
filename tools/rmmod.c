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
#include <sys/utsname.h>

#include <shared/util.h>
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

static int get_module_dirname(char *dirname_buf, size_t dirname_size,
			      const char *module_directory)
{
	struct utsname u;
	int n;

	if (uname(&u) < 0)
		return EXIT_FAILURE;

	n = snprintf(dirname_buf, dirname_size,
		     "%s/%s", module_directory, u.release);
	if (n >= (int)dirname_size) {
		ERR("bad directory %s/%s: path too long\n",
		    module_directory, u.release);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static int _do_rmmod(const char *dirname, int argc, char *argv[], int flags,
		     int verbose)
{
	struct kmod_ctx *ctx = NULL;
	const char *null_config = NULL;
	int r, i;

	ctx = kmod_new(dirname, &null_config);
	if (!ctx) {
		ERR("kmod_new() failed!\n");
		r = EXIT_FAILURE;
		goto finish;
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

finish:
	kmod_unref(ctx);
	return r;
}

static int do_rmmod(int argc, char *argv[])
{
	char dirname_buf[PATH_MAX];
	int verbose = LOG_ERR;
	int use_syslog = 0;
	int flags = 0;
	int c, r = 0;

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

	r = get_module_dirname(dirname_buf, sizeof(dirname_buf),
			       MODULE_DIRECTORY);
	if (r)
		goto done;
	r = _do_rmmod(dirname_buf, argc, argv, flags, verbose);

done:
	log_close();

	return r;
}

const struct kmod_cmd kmod_cmd_compat_rmmod = {
	.name = "rmmod",
	.cmd = do_rmmod,
	.help = "compat rmmod command",
};
