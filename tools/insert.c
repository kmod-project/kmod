/*
 * kmod-insert - insert a module into the kernel.
 *
 * Copyright (C) 2015 Intel Corporation. All rights reserved.
 * Copyright (C) 2011-2013  ProFUSION embedded systems
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

#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#include <libkmod/libkmod.h>

#include "kmod.h"

static const char cmdopts_s[] = "h";
static const struct option cmdopts[] = {
	{"help", no_argument, 0, 'h'},
	{ }
};

static void help(void)
{
	printf("Usage:\n"
	       "\t%s insert [options] module\n"
	       "Options:\n"
	       "\t-h, --help        show this help\n",
	       program_invocation_short_name);
}

static const char *mod_strerror(int err)
{
	switch (err) {
	case KMOD_PROBE_APPLY_BLACKLIST:
		return "Module is blacklisted";
	case -EEXIST:
		return "Module already in kernel";
	case -ENOENT:
		return "Unknown symbol in module or unknown parameter (see dmesg)";
	default:
		return strerror(-err);
	}
}

static int do_insert(int argc, char *argv[])
{
	struct kmod_ctx *ctx;
	struct kmod_list *list = NULL, *l;
	const char *name;
	int err, r = EXIT_SUCCESS;

	for (;;) {
		int c, idx = 0;
		c = getopt_long(argc, argv, cmdopts_s, cmdopts, &idx);
		if (c == -1)
			break;
		switch (c) {
		case 'h':
			help();
			return EXIT_SUCCESS;
		default:
			ERR("Unexpected getopt_long() value '%c'.\n", c);
			return EXIT_FAILURE;
		}
	}

	if (optind >= argc) {
		ERR("Missing module name\n");
		return EXIT_FAILURE;
	}

	ctx = kmod_new(NULL, NULL);
	if (!ctx) {
		ERR("kmod_new() failed!\n");
		return EXIT_FAILURE;
	}

	name = argv[optind];
	err = kmod_module_new_from_lookup(ctx, name, &list);
	if (err < 0) {
		ERR("Could not lookup module matching '%s': %s\n", name, strerror(-err));
		r = EXIT_FAILURE;
		goto end;
	}

	if (list == NULL) {
		ERR("No module matches '%s'\n", name);
		r = EXIT_FAILURE;
		goto end;
	}

	kmod_list_foreach(l, list) {
		struct kmod_module *mod = kmod_module_get_module(l);

		err = kmod_module_probe_insert_module(mod, KMOD_PROBE_APPLY_BLACKLIST, NULL, NULL, NULL, NULL);
		if (err != 0) {
			r = EXIT_FAILURE;
			ERR("Could not insert '%s': %s\n", kmod_module_get_name(mod), mod_strerror(err));
		}

		kmod_module_unref(mod);
	}

	kmod_module_unref_list(list);
end:
	kmod_unref(ctx);
	return r;
}

const struct kmod_cmd kmod_cmd_insert = {
	.name = "insert",
	.cmd = do_insert,
	.help = "insert a module into the kernel",
};
