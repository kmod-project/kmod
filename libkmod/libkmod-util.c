/*
 * libkmod - interface to kernel module operations
 *
 * Copyright (C) 2011-2013  ProFUSION embedded systems
 * Copyright (C) 2012  Lucas De Marchi <lucas.de.marchi@gmail.com>
 * Copyright (C) 2013  Intel Corporation. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
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
#include <string.h>
#include <ctype.h>

#include <shared/util.h>

#include "libkmod.h"
#include "libkmod-internal.h"

const struct kmod_ext kmod_exts[] = {
	{".ko", sizeof(".ko") - 1},
#ifdef ENABLE_ZLIB
	{".ko.gz", sizeof(".ko.gz") - 1},
#endif
#ifdef ENABLE_XZ
	{".ko.xz", sizeof(".ko.xz") - 1},
#endif
	{ }
};

inline int alias_normalize(const char *alias, char buf[PATH_MAX], size_t *len)
{
	size_t s;

	for (s = 0; s < PATH_MAX - 1; s++) {
		const char c = alias[s];
		switch (c) {
		case '-':
			buf[s] = '_';
			break;
		case ']':
			return -EINVAL;
		case '[':
			while (alias[s] != ']' && alias[s] != '\0') {
				buf[s] = alias[s];
				s++;
			}

			if (alias[s] != ']')
				return -EINVAL;

			buf[s] = alias[s];
			break;
		case '\0':
			goto finish;
		default:
			buf[s] = c;
		}
	}

finish:
	buf[s] = '\0';

	if (len)
		*len = s;

	return 0;
}

inline char *modname_normalize(const char *modname, char buf[PATH_MAX],
								size_t *len)
{
	size_t s;

	for (s = 0; s < PATH_MAX - 1; s++) {
		const char c = modname[s];
		if (c == '-')
			buf[s] = '_';
		else if (c == '\0' || c == '.')
			break;
		else
			buf[s] = c;
	}

	buf[s] = '\0';

	if (len)
		*len = s;

	return buf;
}

char *path_to_modname(const char *path, char buf[PATH_MAX], size_t *len)
{
	char *modname;

	modname = basename(path);
	if (modname == NULL || modname[0] == '\0')
		return NULL;

	return modname_normalize(modname, buf, len);
}

bool path_ends_with_kmod_ext(const char *path, size_t len)
{
	const struct kmod_ext *eitr;

	for (eitr = kmod_exts; eitr->ext != NULL; eitr++) {
		if (len <= eitr->len)
			continue;
		if (streq(path + len - eitr->len, eitr->ext))
			return true;
	}

	return false;
}
