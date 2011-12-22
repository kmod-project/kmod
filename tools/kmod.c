/*
 * kmod - one tool to rule them all
 *
 * Copyright (C) 2011  ProFUSION embedded systems
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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <string.h>
#include <libkmod.h>
#include "kmod.h"

static const char options_s[] = "+hV";
static const struct option options[] = {
	{ "help", no_argument, NULL, 'h' },
	{ "version", no_argument, NULL, 'V' },
	{}
};

static const struct kmod_cmd kmod_cmd_help;

static const struct kmod_cmd *kmod_cmds[] = {
	&kmod_cmd_help,
};

static int kmod_help(int argc, char *argv[])
{
	size_t i;

	printf("Manage kernel modules: list, load, unload, etc\n"
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

	puts("\nkmod will also handle gracefully if called\n"
			"from a symlink to previous tools\n");

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
			puts("kmod version " VERSION);
			return EXIT_SUCCESS;
		case '?':
			return EXIT_FAILURE;
		default:
			fprintf(stderr, "Error: unexpected getopt_long() value '%c'.\n", c);
			return EXIT_FAILURE;
		}
	}

	if (optind >= argc) {
		err = -ENOENT;
		goto finish;
	}

	cmd = argv[optind];

	for (i = 0; i < ARRAY_SIZE(kmod_cmds); i++) {
		if (strcmp(kmod_cmds[i]->name, cmd) != 0)
			continue;

		err = kmod_cmds[i]->cmd(--argc, ++argv);
	}

finish:
	if (err < 0) {
		fputs("missing or unknown command; "
			"see 'kmod help' for a list of available commands\n", stderr);
	}

	return err;
}

int main(int argc, char *argv[])
{
	const char *binname = basename(argv[0]);
	int err;

	if (strcmp(binname, "kmod") == 0)
		err = handle_kmod_commands(argc, argv);
	else
		err = -ENOENT;

	return err;
}
