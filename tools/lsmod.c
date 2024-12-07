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
#include <sys/utsname.h>

#include <shared/util.h>

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

static int _do_lsmod(const char *dirname, int verbose)
{
	struct kmod_ctx *ctx = NULL;
	const char *null_config = NULL;
	struct kmod_list *list, *itr;
	int r, err;

	ctx = kmod_new(dirname, &null_config);
	if (!ctx) {
		ERR("kmod_new() failed!\n");
		r = EXIT_FAILURE;
		goto finish;
	}

	log_setup_kmod_log(ctx, verbose);

	err = kmod_module_new_from_loaded(ctx, &list);
	if (err < 0) {
		ERR("could not get list of modules: %s\n", strerror(-err));
		r = EXIT_FAILURE;
		goto finish;
	}

	puts("Module                  Size  Used by");

	kmod_list_foreach(itr, list) {
		struct kmod_module *mod = kmod_module_get_module(itr);
		const char *name = kmod_module_get_name(mod);
		int use_count = kmod_module_get_refcnt(mod);
		long size = kmod_module_get_size(mod);
		struct kmod_list *holders, *hitr;
		int first = 1;

		printf("%-19s %8ld  %d", name, size, use_count);
		holders = kmod_module_get_holders(mod);
		kmod_list_foreach(hitr, holders) {
			struct kmod_module *hm = kmod_module_get_module(hitr);

			if (!first) {
				putchar(',');
			} else {
				putchar(' ');
				first = 0;
			}

			fputs(kmod_module_get_name(hm), stdout);
			kmod_module_unref(hm);
		}
		putchar('\n');
		kmod_module_unref_list(holders);
		kmod_module_unref(mod);
	}
	kmod_module_unref_list(list);

finish:
	kmod_unref(ctx);
	return r;
}

static int do_lsmod(int argc, char *argv[])
{
	char dirname_buf[PATH_MAX];
	int verbose = LOG_ERR;
	int use_syslog = 0;
	int c, r = 0;

	while ((c = getopt_long(argc, argv, cmdopts_s, cmdopts, NULL)) != -1) {
		switch (c) {
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

	if (optind < argc) {
		ERR("too many arguments provided.\n");
		r = EXIT_FAILURE;
		goto done;
	}

	/* Try first with MODULE_DIRECTORY */
	r = get_module_dirname(dirname_buf, sizeof(dirname_buf),
			       MODULE_DIRECTORY);
	if (r)
		goto done;
	r = _do_lsmod(dirname_buf, verbose);
	if (!r)
		goto done;

	/* If not found, look at MODULE_ALTERNATIVE_DIRECTORY */
	r = get_module_dirname(dirname_buf, sizeof(dirname_buf),
			       MODULE_ALTERNATIVE_DIRECTORY);
	if (r)
		goto done;
	r = _do_lsmod(dirname_buf, verbose);

done:
	log_close();

	return r;
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
