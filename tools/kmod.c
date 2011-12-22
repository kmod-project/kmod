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
#include <string.h>
#include <libkmod.h>
#include "kmod.h"

static const struct kmod_cmd kmod_cmd_help;

static const struct kmod_cmd *kmod_cmds[] = {
	&kmod_cmd_help,
};

static int kmod_help(int argc, char *argv[])
{
	size_t i;

	puts("Manage kernel modules: list, load, unload, etc\n"
			"Usage: kmod COMMAND [COMMAND_OPTIONS]\n\n"
			"Available commands:");

	for (i = 0; i < ARRAY_SIZE(kmod_cmds); i++) {
		if (kmod_cmds[i]->help != NULL) {
			printf("  %-12s %s\n", kmod_cmds[i]->name,
							kmod_cmds[i]->help);
		}
	}

	return EXIT_SUCCESS;
}

static const struct kmod_cmd kmod_cmd_help = {
	.name = "help",
	.cmd = kmod_help,
	.help = "Show help message",
};

int main(int argc, char *argv[])
{
	const char *cmd;
	int err = 0;
	size_t i;

	if (argc < 2) {
		err = -ENOENT;
		goto finish;
	}

	cmd = argv[1];

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
