/*
 * libkmod - interface to kernel module operations
 *
 * Copyright (C) 2011  ProFUSION embedded systems
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation version 2.1.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>

#include "libkmod.h"
#include "libkmod-private.h"

/**
 * SECTION:libkmod-loaded
 * @short_description: currently loaded modules
 *
 * Information about currently loaded modules, as reported by Linux kernel
 */
KMOD_EXPORT int kmod_loaded_get_list(struct kmod_ctx *ctx,
						struct kmod_list **list)
{
	struct kmod_list *l = NULL;
	FILE *fp;
	char line[4096];

	if (ctx == NULL || list == NULL)
		return -ENOENT;

	fp = fopen("/proc/modules", "r");
	if (fp == NULL) {
		int err = -errno;
		ERR(ctx, "could not open /proc/modules: %s\n", strerror(errno));
		return err;
	}

	while (fgets(line, sizeof(line), fp)) {
		struct kmod_module *m;
		struct kmod_list *node;
		int err;
		char *saveptr, *name = strtok_r(line, " \t", &saveptr);

		err = kmod_module_new_from_name(ctx, name, &m);
		if (err < 0) {
			ERR(ctx, "could not get module from name '%s': %s\n",
				name, strerror(-err));
			continue;
		}

		node = kmod_list_append(l, m);
		if (node)
			l = node;
		else {
			ERR(ctx, "out of memory\n");
			kmod_module_unref(m);
		}
	}

	fclose(fp);
	*list = l;

	return 0;
}
