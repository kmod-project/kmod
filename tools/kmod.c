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
#include <shared/missing.h>

#include <libkmod/libkmod.h>

#include "kmod.h"

static const char options_s[] = "+hV";
static const struct option options[] = {
	{ "help", no_argument, NULL, 'h' },
	{ "version", no_argument, NULL, 'V' },
	{},
};

static const struct kmod_cmd kmod_cmd_help;

static const struct kmod_cmd *kmod_cmds[] = {
	&kmod_cmd_help,
	&kmod_cmd_list,
	&kmod_cmd_static_nodes,
};

static const struct kmod_cmd *kmod_compat_cmds[] = {
	// clang-format off
	&kmod_cmd_compat_lsmod,
	&kmod_cmd_compat_rmmod,
	&kmod_cmd_compat_insmod,
	&kmod_cmd_compat_modinfo,
	&kmod_cmd_compat_modprobe,
	&kmod_cmd_compat_depmod,
	// clang-format on
};

static int kmod_help(int argc, char *argv[])
{
	size_t i;

	printf("kmod - Manage kernel modules: list, load, unload, etc\n"
			"Usage:\n"
			"\t%s [options] command [command_options]\n\n"
			"Options:\n"
			"\t-V, --version     show version\n"
			"\t-h, --help        show this help\n\n"
			"Commands:\n", basename(argv[0]));

	for (i = 0; i < ARRAY_SIZE(kmod_cmds); i++) {
		if (kmod_cmds[i]->help != NULL) {
			printf("  %-12s %s\n", kmod_cmds[i]->name,
							kmod_cmds[i]->help);
		}
	}

	puts("\nkmod also handles gracefully if called from following symlinks:");

	for (i = 0; i < ARRAY_SIZE(kmod_compat_cmds); i++) {
		if (kmod_compat_cmds[i]->help != NULL) {
			printf("  %-12s %s\n", kmod_compat_cmds[i]->name,
						kmod_compat_cmds[i]->help);
		}
	}

	return EXIT_SUCCESS;
}

static const struct kmod_cmd kmod_cmd_help = {
	.name = "help",
	.cmd = kmod_help,
	.help = "Show help message",
};

static int handle_kmod_commands(int argc, char *argv[])
{
	const char *cmd;
	int err = 0;
	size_t i;

	for (;;) {
		int c;

		c = getopt_long(argc, argv, options_s, options, NULL);
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			kmod_help(argc, argv);
			return EXIT_SUCCESS;
		case 'V':
			puts(PACKAGE " version " VERSION);
			puts(KMOD_FEATURES);
			return EXIT_SUCCESS;
		case '?':
			return EXIT_FAILURE;
		default:
			fprintf(stderr, "Error: unexpected getopt_long() value '%c'.\n", c);
			return EXIT_FAILURE;
		}
	}

	if (optind >= argc) {
		fputs("missing command\n", stderr);
		goto fail;
	}

	cmd = argv[optind];

	for (i = 0, err = -EINVAL; i < ARRAY_SIZE(kmod_cmds); i++) {
		if (streq(kmod_cmds[i]->name, cmd)) {
			err = kmod_cmds[i]->cmd(--argc, ++argv);
			break;
		}
	}

	if (err < 0) {
		fprintf(stderr, "invalid command '%s'\n", cmd);
		goto fail;
	}

	return err;

fail:
	kmod_help(argc, argv);
	return EXIT_FAILURE;
}


static int handle_kmod_compat_commands(int argc, char *argv[])
{
	const char *cmd;
	size_t i;

	cmd = basename(argv[0]);

	for (i = 0; i < ARRAY_SIZE(kmod_compat_cmds); i++) {
		if (streq(kmod_compat_cmds[i]->name, cmd))
			return kmod_compat_cmds[i]->cmd(argc, argv);
	}

	return EXIT_FAILURE;
}

int main(int argc, char *argv[])
{
	int err;

	if (streq(program_invocation_short_name, "kmod"))
		err = handle_kmod_commands(argc, argv);
	else
		err = handle_kmod_compat_commands(argc, argv);

	return err;
}
