// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2011-2013  ProFUSION embedded systems
 */

#include <errno.h>
#include <getopt.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libkmod/libkmod.h>

#include "kmod.h"

static const char cmdopts_s[] = "svVh";
static const struct option cmdopts[] = {
	// clang-format off
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
	       "\t%s [options]\n"
	       "Options:\n"
	       "\t-s, --syslog      print to syslog, not stderr\n"
	       "\t-v, --verbose     enables more messages\n"
	       "\t-V, --version     show version\n"
	       "\t-h, --help        show this help\n",
	       program_invocation_short_name);
}

static int do_lsmod(int argc, char *argv[])
{
	struct kmod_ctx *ctx = NULL;
	const char *null_config = NULL;
	struct kmod_list *list, *itr;
	int verbose = LOG_ERR;
	bool use_syslog = false;
	int err, c, r = 0;

	while ((c = getopt_long(argc, argv, cmdopts_s, cmdopts, NULL)) != -1) {
		switch (c) {
		case 's':
			use_syslog = true;
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

	if (optind < argc) {
		ERR("too many arguments provided.\n");
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

	err = kmod_module_new_from_loaded(ctx, &list);
	if (err < 0) {
		ERR("could not get list of modules: %s\n", strerror(-err));
		r = EXIT_FAILURE;
		goto done;
	}

	puts("Module                  Size  Used by");

	kmod_list_foreach(itr, list) {
		struct kmod_module *mod = kmod_module_get_module(itr);
		const char *name = kmod_module_get_name(mod);
		int use_count = kmod_module_get_refcnt(mod);
		long size = kmod_module_get_size(mod);
		struct kmod_list *holders, *hitr;
		int sep = ' ';

		printf("%-19s %8ld  %d", name, size, use_count);
		holders = kmod_module_get_holders(mod);
		kmod_list_foreach(hitr, holders) {
			struct kmod_module *hm = kmod_module_get_module(hitr);

			putchar(sep);
			sep = ',';

			fputs(kmod_module_get_name(hm), stdout);
			kmod_module_unref(hm);
		}
		putchar('\n');
		kmod_module_unref_list(holders);
		kmod_module_unref(mod);
	}
	kmod_module_unref_list(list);

done:
	kmod_unref(ctx);
	log_close();

	return r == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

const struct kmod_cmd kmod_cmd_compat_lsmod = {
	.name = "lsmod",
	.cmd = do_lsmod,
	.help = "compat lsmod command",
};

const struct kmod_cmd kmod_cmd_list = {
	.name = "list",
	.cmd = do_lsmod,
	.help = "list currently loaded modules",
};
