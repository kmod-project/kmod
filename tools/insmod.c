// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2011-2013  ProFUSION embedded systems
 */

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <shared/util.h>

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
	       "\t%s [options] filename [args]\n"
	       "Options:\n"
	       "\t-f, --force       DANGEROUS: forces a module load, may cause\n"
	       "\t                  data corruption and crash your machine\n"
	       "\t-s, --syslog      print to syslog, not stderr\n"
	       "\t-v, --verbose     enables more messages\n"
	       "\t-V, --version     show version\n"
	       "\t-h, --help        show this help\n",
	       program_invocation_short_name);
}

static const char *mod_strerror(int err)
{
	switch (err) {
	case ENOEXEC:
		return "Invalid module format";
	case ENOENT:
		return "Unknown symbol in module";
	case ESRCH:
		return "Module has wrong symbol version";
	case EINVAL:
		return "Invalid parameters";
	default:
		return strerror(err);
	}
}

static int do_insmod(int argc, char *argv[])
{
	struct kmod_ctx *ctx = NULL;
	struct kmod_module *mod;
	const char *filename;
	char *opts = NULL;
	size_t optslen = 0;
	int verbose = LOG_ERR;
	int use_syslog;
	int i, err = 0, r = 0;
	const char *null_config = NULL;
	unsigned int flags = 0;

	for (;;) {
		int c, idx = 0;
		c = getopt_long(argc, argv, cmdopts_s, cmdopts, &idx);
		if (c == -1)
			break;
		switch (c) {
		case 'f':
			flags |= KMOD_PROBE_FORCE_MODVERSION;
			flags |= KMOD_PROBE_FORCE_VERMAGIC;
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
		ERR("missing filename.\n");
		r = EXIT_FAILURE;
		goto end;
	}

	filename = argv[optind];
	if (streq(filename, "-")) {
		ERR("this tool does not support loading from stdin!\n");
		r = EXIT_FAILURE;
		goto end;
	}

	for (i = optind + 1; i < argc; i++) {
		size_t len = strlen(argv[i]);
		void *tmp = realloc(opts, optslen + len + 2);
		if (tmp == NULL) {
			ERR("out of memory\n");
			r = EXIT_FAILURE;
			goto end;
		}
		opts = tmp;
		if (optslen > 0) {
			opts[optslen] = ' ';
			optslen++;
		}
		memcpy(opts + optslen, argv[i], len);
		optslen += len;
		opts[optslen] = '\0';
	}

	ctx = kmod_new(NULL, &null_config);
	if (!ctx) {
		ERR("kmod_new() failed!\n");
		r = EXIT_FAILURE;
		goto end;
	}

	log_setup_kmod_log(ctx, verbose);

	err = kmod_module_new_from_path(ctx, filename, &mod);
	if (err < 0) {
		ERR("could not load module %s: %s\n", filename, strerror(-err));
		r++;
		goto end;
	}

	err = kmod_module_insert_module(mod, flags, opts);
	if (err < 0) {
		ERR("could not insert module %s: %s\n", filename, mod_strerror(-err));
		r++;
	}
	kmod_module_unref(mod);

end:
	kmod_unref(ctx);
	free(opts);

	log_close();
	return err >= 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

const struct kmod_cmd kmod_cmd_compat_insmod = {
	.name = "insmod",
	.cmd = do_insmod,
	.help = "compat insmod command",
};
