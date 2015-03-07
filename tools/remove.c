/*
 * kmod-remove - remove modules from the kernel.
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
	       "\t%s remove [options] module\n"
	       "Options:\n"
	       "\t-h, --help        show this help\n",
	       program_invocation_short_name);
}

static int check_module_inuse(struct kmod_module *mod) {
	struct kmod_list *holders;
	int state;

	state = kmod_module_get_initstate(mod);

	if (state == KMOD_MODULE_BUILTIN) {
		ERR("Module %s is builtin.\n", kmod_module_get_name(mod));
		return -ENOENT;
	} else if (state < 0) {
		ERR("Module %s is not currently loaded\n",
				kmod_module_get_name(mod));
		return -ENOENT;
	}

	holders = kmod_module_get_holders(mod);
	if (holders != NULL) {
		struct kmod_list *itr;

		ERR("Module %s is in use by:", kmod_module_get_name(mod));

		kmod_list_foreach(itr, holders) {
			struct kmod_module *hm = kmod_module_get_module(itr);
			fprintf(stderr, " %s", kmod_module_get_name(hm));
			kmod_module_unref(hm);
		}
		fputc('\n', stderr);

		kmod_module_unref_list(holders);
		return -EBUSY;
	}

	if (kmod_module_get_refcnt(mod) != 0) {
		ERR("Module %s is in use\n", kmod_module_get_name(mod));
		return -EBUSY;
	}

	return 0;
}

static int do_remove(int argc, char *argv[])
{
	struct kmod_ctx *ctx;
	struct kmod_module *mod;
	const char *name;
	int err, r = EXIT_SUCCESS;

	for (;;) {
		int c, idx =0;
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
	err = kmod_module_new_from_name(ctx, name, &mod);
	if (err < 0) {
		ERR("Could not remove module %s: %s\n", name, strerror(-err));
		goto end;
	}

	err = check_module_inuse(mod);
	if (err < 0)
		goto unref;

	err = kmod_module_remove_module(mod, 0);
	if (err < 0)
		goto unref;

unref:
	kmod_module_unref(mod);

end:
	kmod_unref(ctx);
	if (err < 0) {
		r = EXIT_FAILURE;
		ERR("Could not remove module %s: %s\n", name, strerror(-err));
	}
	return r;
}

const struct kmod_cmd kmod_cmd_remove = {
	.name = "remove",
	.cmd = do_remove,
	.help = "remove module from kernel",
};
