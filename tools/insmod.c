// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2011-2013  ProFUSION embedded systems
 */

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <shared/util.h>

#include <libkmod/libkmod.h>

#include "kmod.h"

static const char cmdopts_s[] = "fsvVh";
static const struct option cmdopts[] = {
	// clang-format off
	{ "force", no_argument, 0, 'f' },
	{ "force-modversion", no_argument, 0, 2 },
	{ "force-vermagic", no_argument, 0, 1 },
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
	       "\t%s [options] filename [module options]\n"
	       "Options:\n"
	       "\t-f, --force              DANGEROUS: forces a module load, may cause\n"
	       "\t                         data corruption and crash your machine.\n"
	       "\t                         implies --force-modversion and\n"
	       "\t                         --force-vermagic\n"
	       "\t    --force-modversion   Ignore module's version\n"
	       "\t    --force-vermagic     Ignore module's version magic\n"
	       "\t-s, --syslog             print to syslog, not stderr\n"
	       "\t-v, --verbose            enables more messages\n"
	       "\t-V, --version            show version\n"
	       "\t-h, --help               show this help\n",
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
	int verbose = LOG_ERR;
	bool use_syslog = false;
	int c, r = 0;
	const char *null_config = NULL;
	unsigned int flags = 0;

	while ((c = getopt_long(argc, argv, cmdopts_s, cmdopts, NULL)) != -1) {
		switch (c) {
		case 'f':
			flags |= KMOD_PROBE_FORCE_MODVERSION;
			flags |= KMOD_PROBE_FORCE_VERMAGIC;
			break;
		case 2:
			flags |= KMOD_PROBE_FORCE_MODVERSION;
			break;
		case 1:
			flags |= KMOD_PROBE_FORCE_VERMAGIC;
			break;
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

	r = options_from_array(argv + optind + 1, argc - optind - 1, &opts);
	if (r < 0)
		goto end;

	ctx = kmod_new(NULL, &null_config);
	if (!ctx) {
		ERR("kmod_new() failed!\n");
		r = EXIT_FAILURE;
		goto end;
	}

	log_setup_kmod_log(ctx, verbose);

	r = kmod_module_new_from_path(ctx, filename, &mod);
	if (r < 0) {
		ERR("could not load module %s: %s\n", filename, strerror(-r));
		goto end;
	}

	r = kmod_module_insert_module(mod, flags, opts);
	if (r < 0)
		ERR("could not insert module %s: %s\n", filename, mod_strerror(-r));

	kmod_module_unref(mod);

end:
	kmod_unref(ctx);
	free(opts);

	log_close();
	return r == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

const struct kmod_cmd kmod_cmd_compat_insmod = {
	.name = "insmod",
	.cmd = do_insmod,
	.help = "compat insmod command",
};
